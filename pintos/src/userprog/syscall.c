#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "lib/stdbool.h"

static void syscall_handler (struct intr_frame *);
int address_is_valid (char *, int size);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  struct thread *current_thread = thread_current();
  uint32_t* args = ((uint32_t*) f->esp);
  uint32_t args_pd = active_pd ();
  void *args_mapped = pagedir_get_page (args_pd, args);
  if (!f || !args_mapped || !address_is_valid (args, sizeof(args))) {
    current_thread->info->exit_code = -1;
    thread_exit();
  }
  if (args[0] == SYS_EXIT) {
    // address_is_valid  (args[1], sizeof(args[1]));
    f->eax = args[1];
    current_thread->info->exit_code = args[1];
    thread_exit();
  } else if (args[0] == SYS_CREATE) {
    // Check memory accesses for all below
    char *file_name = (char *) args[1];
    uint32_t pd = active_pd ();
    void *mapped = pagedir_get_page (pd, file_name);
    if (!mapped || !file_name || !address_is_valid (file_name, strlen(file_name) + 1) || !strcmp(file_name, ""))
      {
        current_thread->info->exit_code = -1;
        thread_exit();
      }
    lock_filesys ();
    bool created = filesys_create (file_name, args[2]); // Create new file with initial size;
    release_filesys ();
    f->eax = created;
  } else if (args[0] == SYS_REMOVE) {
    char *file_name = (char *) args[1];
    uint32_t pd = active_pd ();
    void *mapped = pagedir_get_page (pd, file_name);
    if (!mapped || !file_name || !address_is_valid (file_name, strlen(file_name) + 1))
      {
        current_thread->info->exit_code = -1;
        thread_exit();
      }
    lock_filesys ();
    bool removed = filesys_remove (file_name); // Create new file with initial size;
    release_filesys ();
    f->eax = removed;
  } else if (args[0] == SYS_OPEN) {
    // address_is_valid ((char *) args[1], sizeof((char *) args[1])); // Why is this passing without checking memory?
    char *file_name = (char *) args[1];
    uint32_t pd = active_pd ();
    void *mapped = pagedir_get_page (pd, file_name);
    if (!mapped || !file_name || !address_is_valid ((char *) args[1], strlen((char *) args[1])))
      {
        current_thread->info->exit_code = -1;
        thread_exit();
      }
    else if (!strcmp((char *) args[1], ""))
      f->eax = -1;
    else
      {
        lock_filesys ();
        struct file *file = filesys_open (file_name);
        if (file)
          {
            int fd = insert_file_to_fd_table (file);
            f->eax = fd;
          }
        else
          f->eax = -1;
        release_filesys ();
      }
  } else if (args[0] == SYS_FILESIZE) {
    lock_filesys ();
    struct file *file = get_file (args[1]);
    if (file)
      f->eax = file_length (file);
    else
      f->eax = -1;
    release_filesys ();
  } else if (args[0] == SYS_READ) {
    uint32_t pd = active_pd ();
    if (!address_is_valid ((char *) args[2], args[3]) || !pagedir_get_page (pd, args[2]) ||
        args[1] == 1)
      {
        current_thread->info->exit_code = -1;
        thread_exit();
      }
    lock_filesys ();
    struct file *file = get_file (args[1]);
    if (file)
      f->eax = file_read (file, (char *) args[2], args[3]);
    release_filesys ();
  } else if (args[0] == SYS_WRITE) {
    uint32_t pd = active_pd ();
    if (!address_is_valid ((char *) args[2], args[3]) || !pagedir_get_page (pd, args[2]) ||
        args[1] == 0)
      {
        current_thread->info->exit_code = -1;
        thread_exit();
      }
    lock_filesys ();
    if (args[1] == 1)
      {
        putbuf((char *) args[2], args[3]);
      }
    else
      {
        struct file *file = get_file (args[1]);
        if (file)
          f->eax = file_write (file, (char *) args[2], args[3]);
      }
    release_filesys ();
  } else if (args[0] == SYS_SEEK) {
    lock_filesys ();
    struct file *file = get_file (args[1]);
    file_seek (file, args[2]);
    release_filesys ();
  } else if (args[0] == SYS_TELL) {
    lock_filesys ();
    struct file *file = get_file (args[1]);
    f->eax = file_tell (file);
    release_filesys ();
  } else if (args[0] == SYS_CLOSE) {
    lock_filesys ();
    struct file *file = get_file (args[1]);
    if (file && !file->deny_write)
      {
        close_file (args[1]);
        file_close (file);
      }
    release_filesys ();
  } else if (args[0] == SYS_PRACTICE) {
    address_is_valid (args[1], sizeof(args[1]));
    f->eax = args[1] + 1;
  } else if (args[0] == SYS_HALT) {
    shutdown_power_off();
  } else if (args[0] == SYS_EXEC) {
    char *file_name = (char *) args[1];
    uint32_t pd = active_pd ();
    void *mapped = pagedir_get_page (pd, file_name);
    if (!mapped || !file_name || !address_is_valid ((char *) args[1], strlen((char *) args[1])))
      {
        current_thread->info->exit_code = -1;
        thread_exit();
      }
    // add in a semaphore to args[1], which is the filename/arguments to be executed
    f->eax = process_execute((char *) args[1]);
    // wait for above to execute by:
    // trying to down a sempahore that will only be upped when process_execute is finished
  } else if (args[0] == SYS_WAIT) {
    // address_is_valid (args[1], sizeof(args[1]));
    f->eax = process_wait(args[1]);
  }
}

/* Checks if the address is null and if the address
   and its size fits in the user address space. Returns 0 if an error occurred,
   1 if it didnt */
int
address_is_valid (char *addr, int size) {
  if (addr < 0x08048000 || !is_user_vaddr(addr + size)) {
      // struct thread *current_thread = thread_current ();
      // current_thread->info->exit_code = -1;
      return 0;
  }
}