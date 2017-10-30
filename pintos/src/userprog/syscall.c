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
  uint32_t* args = ((uint32_t*) f->esp);
  if (!address_is_valid (args, sizeof(args))) {
    printf("%s: exit(%d)\n", (char *) &thread_current ()->name, -1); // Change to exit code later on and print in thread_exit.
    thread_exit();
  }
  struct thread *current_thread = thread_current();
  if (args[0] == SYS_EXIT) {
    // address_is_valid  (args[1], sizeof(args[1]));
    f->eax = args[1];
    printf("%s: exit(%d)\n", (char *) &thread_current ()->name, args[1]);
    current_thread->info->exit_code = args[1];
    thread_exit();
  } else if (args[0] == SYS_CREATE) {
    // Check memory accesses for all below
    char *file_name = (char *) args[1];
    address_is_valid (file_name, sizeof(file_name));
    lock_filesys ();
    bool created = filesys_create (file_name, 1024); // Create new file with initial size;
    release_filesys ();
    f->eax = created;
  } else if (args[0] == SYS_REMOVE) {
    char *file_name = (char *) args[1];
    address_is_valid (file_name, sizeof(file_name));
    lock_filesys ();
    bool removed = filesys_remove (file_name); // Create new file with initial size;
    release_filesys ();
    f->eax = removed;
  } else if (args[0] == SYS_OPEN) {
    // address_is_valid ((char *) args[1], sizeof((char *) args[1])); // Why is this passing without checking memory?
    char *file_name = (char *) args[1];
    if (!address_is_valid ((char *) args[1], sizeof((char *) args[1])) || !strcmp((char *) args[1], ""))
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
    f->eax = file_length (file);
    release_filesys ();
  } else if (args[0] == SYS_READ) {
    address_is_valid ((char *) args[2], args[3]);
    lock_filesys ();
    struct file *file = get_file (args[1]);
    f->eax = file_read (file, (char *) args[2], args[3]);
    release_filesys ();
  } else if (args[0] == SYS_WRITE) {
    address_is_valid ((char *) args[2], args[3]);
    lock_filesys ();
    if (args[1] == 1)
      {
        putbuf((char *) args[2], args[3]);
      }
    else
      {
        struct file *file = get_file (args[1]);
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
    if (file)
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
    address_is_valid (args[1], sizeof((char *) args[1]));
    // add in a semaphore to args[1], which is the filename/arguments to be executed
    process_execute((char *) args[1]);
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
  if (addr == NULL || addr < 0x08048000 || !is_user_vaddr(addr + size)) {
      // struct thread *current_thread = thread_current ();
      // current_thread->info->exit_code = -1;
      return 0;
  }
}