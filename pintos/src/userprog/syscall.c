#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
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
  check_memory_access (f->esp);
  char** args = ((char**) f->esp);
  printf("System call number: %d\n", args[0]);
  if (args[0] == SYS_EXIT) {
    struct thread *current_thread = thread_current();
  	check_memory_access (args[1]);
    f->eax = args[1];
    printf("%s: exit(%d)\n", &thread_current ()->name, args[1]);
    current_thread->info->exit_code = args[1];
    thread_exit();
  } else if (args[0] == SYS_PRACTICE) {
  	check_memory_access (args[1]);
  	f->eax = args[1] + 1;
  } else if (args[0] == SYS_HALT) {
  	shutdown_power_off();
  } else if (args[0] == SYS_EXEC) {
  	check_memory_access (args[1]);
  	// add in a semaphore to args[1], which is the filename/arguments to be executed
  	process_execute(args[1]);
  	// wait for above to execute by:
  	// trying to down a sempahore that will only be upped when process_execute is finished
  } else if (args[0] == SYS_WAIT) {
  	check_memory_access (args[1]);
  	// check that the pid is actually a child of the parent
  	// keep track of the children?
  	process_wait(args[1]);
  }
}

void
check_memory_access (char **addr) {
  if (addr == NULL || !is_user_vaddr(addr + sizeof(addr))) {
      struct thread *current_thread = thread_current ();
      sema_down (current_thread->info->shared_sema);
      current_thread->info->exit_code = -1;
      sema_up (current_thread->info->shared_sema);
      thread_exit ();

  }
}
