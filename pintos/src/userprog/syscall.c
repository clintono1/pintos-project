#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"

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
  check_memory_access (args, sizeof(args));
  printf("System call number: %d\n", args[0]);
  struct thread *current_thread = thread_current();
  if (args[0] == SYS_EXIT) {
  	check_memory_access (args[1], sizeof(args[1]));
    f->eax = args[1];
    printf("%s: exit(%d)\n", (char *) &thread_current ()->name, args[1]);
    current_thread->info->exit_code = args[1];
    thread_exit();
  } else if (args[0] == SYS_PRACTICE) {
  	check_memory_access (args[1], sizeof(args[1]));
  	f->eax = args[1] + 1;
  } else if (args[0] == SYS_HALT) {
  	shutdown_power_off();
  } else if (args[0] == SYS_EXEC) {
  	check_memory_access (args[1], sizeof((char *) args[1]));
  	// add in a semaphore to args[1], which is the filename/arguments to be executed
  	process_execute((char *) args[1]);
  	// wait for above to execute by:
  	// trying to down a sempahore that will only be upped when process_execute is finished
  } else if (args[0] == SYS_WAIT) {
  	check_memory_access (args[1], sizeof(args[1]));
    f->eax = process_wait(args[1]);
  }
}

void
check_memory_access (char *addr, int size) {
  if (addr == NULL || !is_user_vaddr(addr + size)) {
      struct thread *current_thread = thread_current ();
      current_thread->info->exit_code = -1;
      thread_exit ();
  }
}
