#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

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
  if (is_user_vaddr(arg[0])) {
  	// terminate user process
  }
  printf("System call number: %d\n", args[0]);
  if (args[0] == SYS_EXIT) {
  	if (is_user_vaddr(arg[1])) {
  	  // terminate user process
  	}
    f->eax = args[1];
    printf("%s: exit(%d)\n", &thread_current ()->name, args[1]);
    thread_exit();
  } else if (args[0] == SYS_PRACTICE) {
  	if (is_user_vaddr(arg[1])) {
  	  // terminate user process
  	}
  	printf("%d", args[1]++);
  } else if (args[0] == SYS_HALT) {
  	shutdown_power_off();
  } else if (args[0] == SYS_EXEC) {
  	if (is_user_vaddr(arg[1])) {
  	  // terminate user process
  	}
  	// add in a semaphore to args[1], which is the filename/arguments to be executed
  	process_execute(args[1]);
  	// wait for above to execute by:
  	// trying to down a sempahore that will only be upped when process_execute is finished
  } else if (args[0] == SYS_WAIT) {
  	if (is_user_vaddr(arg[1])) {
  	  // terminate user process
  	}
  	// check that the pid is actually a child of the parent
  	// keep track of the children?
  	process_wait(args[1]);
  }
}
