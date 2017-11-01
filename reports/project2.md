Final Report for Project 1: User Programs
===================================

# Changes Since Initial Design Document

## Part 1: Argument Passing
We had to parse the string in `process_execute` before `thread_create`
was called so that it would obtain the correct value for its first argument,
which is supposed to only be the name of the program. We also made sure to push
the arguments onto the stack inside `process_execute` after `load` was called
so that the stack could be set up first. Then we followed our
design document and pushed the arguments onto the stack as was specified
in the project specifications.

## Part 2: Process Control Syscalls
We had to modify the child_info struct so that it looks like

```
struct child_info
  {
    /* Used to pass info between parent and child thread */
    struct semaphore *wait_semaphore;
    struct semaphore *wait_child_exec;   /* A reference to the semaphore used to release parent from
                                            waiting for child to finish loading */
    int *process_loaded;
    int exit_code;
    pid_t pid;
    struct list_elem elem;
    int counter;
  };
```
The wait_semaphore is used by a parent to wait for a child and the
wait_child_exec member is used to wait for a child to finish loading. The
process_loaded member is how the child tells the parent whether it successfully
loaded or not. Thus, using wait_child_exec and process_loaded, we can force
the parent process to wait for its child to finish loading and return -1 if it
failed. For exec, we have to make sure that the string is inside the user's
virtual address space and that it has been instantiated, or else we terminate
the parent process. We also initialize exit codes to be -1. Since all programs
that exit successfully will use the exit() syscall, we change the exit code
when handling that syscall in syscall_handler.

## Part 3: File Operation Syscalls
We changed our implmentation of the fd_table to be a member in the thread
struct, since each process can have up to 128 files open. We have dummy values
for the first two indices to represent stdin and stdout. We also keep track
of `latest_fd` inside the thread struct so we know where to start finding
the next available file descriptor from. The fd_table also points to file
pointers rather than the files themselves since it is easier to deal with
the pointers. We also place a file pointer inside the thread struct
that points to its own executable, so that it can lock that file and release
it when it is about to exit. This allows us to prevent the executable
from being written while it is being run. Many of these syscalls can have
invalid addresses being passed to it so we also have to check them before
we execute them. All of this is checked and handled in the syscall handler.
For **open** and **close** syscalls, we must also add or remove the file object
to the running thread's fd_table. This can be done through the
`insert_file_to_fd_table` and `close_file` functions that we defined in thead.c
which automatically gets the next available fd and inserts it or removes it
(this automatically wraps around to fd 2 if we reach 128, so we can reuse fds).
Since we accept a string in some of the syscalls, like in exec, we also have
to check if the top and bottom of the passed in buffer are valid user addresses
(not unmapped, all of it fits in the user virtual address space, and not NULL).
When we exit a thread, we also make sure to close all of its open files.

# Project Reflection

## Work Distribution

#### Aaron Xu

#### Aidan Meredith

#### Louise Feng

#### Yena Kim

## Things That Went Well

## Things That Could've Been Improved
