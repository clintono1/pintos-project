#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "threads/vaddr.h"


static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  uint32_t* args = ((uint32_t*) f->esp);
  check_memory_access(args, sizeof(args));
  if (args[0] == SYS_EXIT) {
    f->eax = args[1];
    printf("%s: exit(%d)\n", &thread_current ()->name, args[1]);
    thread_exit();
  } else if (args[0] == SYS_CREATE) {
    // Check memory accesses for all below
    check_memory_access(args[1], sizeof((char *) args[1]));
    char *file_name = (char *) args[1];
    lock_filesys ();
    bool created = filesys_create (file_name, 1024); // Create new file with initial size;
    release_filesys ();
    f->eax = created;
  } else if (args[0] == SYS_REMOVE) {
    check_memory_access(args[1], sizeof((char *) args[1]));
    char *file_name = (char *) args[1];
    lock_filesys ();
    bool removed = filesys_create (file_name); // Create new file with initial size;
    release_filesys ();
    f->eax = removed;
  } else if (args[0] == SYS_OPEN) {
    // check_memory_access((char *) args[1], sizeof((char *) args[1])); // Why is this passing without checking memory?
    char *file_name = (char *) args[1];
    lock_filesys ();
    struct file *file = filesys_open (filesys_open); // Create new file with initial size;
    int fd = insert_file_to_fd_table (file);
    release_filesys ();
    f->eax = fd;
  } else if (args[0] == SYS_FILESIZE) {
    lock_filesys ();
    struct file *file = get_file (args[1]);
    f->eax = file_length (file);
    release_filesys ();
  } else if (args[0] == SYS_READ) {
    check_memory_access(args[2], args[3]);
    lock_filesys ();
    struct file *file = get_file (args[1]);
    f->eax = file_read (file, args[2], args[3]);
    release_filesys ();
  } else if (args[0] == SYS_WRITE) {
    check_memory_access(args[2], args[3]);
    lock_filesys ();
    if (args[1] == 1)
      {
        putbuf(args[2], args[3]);
      }
    else
      {
        struct file *file = get_file (args[1]);
        f->eax = file_write (file, args[2], args[3]);
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
    file_close (file);
    release_filesys ();
  }
}

void
check_memory_access (char *addr, int size) {
  if (addr == NULL || !is_user_vaddr(addr + size)) {
      // struct thread *current_thread = thread_current ();
      // current_thread->info->exit_code = -1;
      thread_exit ();
  }
}
