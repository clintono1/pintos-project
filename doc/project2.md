Design Document for Project 2: User Programs
============================================

## Group Members


* Louise Feng <louise.feng@berkeley.edu>
* Yena Kim <yenakim@berkeley.edu>
* Aaron Xu <aaxu@berkeley.edu>
* Aidan Meredith <aidan_meredith@berkeley.edu>

## Task 1: Argument Passing
### Data Structures and Functions
### Algorithms
### Synchronization
### Rationale
## Task 2: Process Control Syscalls
### Data Structures and Functions
### Algorithms
### Synchronization
### Rationale
## Task 3: File Operation Syscalls
### Data Structures and Functions
### Algorithms
### Synchronization
### Rationale
## Design Doc Additional Questions


## GDB Questions

1. The name of the thread is **main** and the thread is at address
   **0xc000e000**. The only other thread present at this time is the **idle**
   thread at address **0xc0104000**.
   **struct thread of main**:
   	    ```
   	    0xc000e000 {tid = 1, status = THREAD_RUNNING,
   	    name = "main", '\000' <repeats 11 times>,
   	    stack = 0xc000ee0c "\210", <incomplete sequence \357>, priority = 31,
   	    allelem = {prev = 0xc0034b50 <all_list>, next = 0xc0104020},
   	    elem = {prev = 0xc0034b60 <ready_list>,
   	    next = 0xc0034b68 <ready_list+8>},
   	    pagedir = 0x0, magic = 3446325067}
   	    ```

   	**struct thread of idle**:
   	    ```
		0xc0104000 {tid = 2, status = THREAD_BLOCKED,
        name = "idle", '\000' <repeats 11 times>,
        stack = 0xc0104f34 "", priority = 0,
        allelem = {prev = 0xc000e020, next = 0xc0034b58 <all_list+8>},
        elem = {prev = 0xc0034b60 <ready_list>,
        next = 0xc0034b68 <ready_list+8>},
        pagedir = 0x0, magic = 3446325067}
   	    ```

2. **Backtrace of current thread**
	#0  process_execute (file_name=file_name@entry=0xc0007d50 "args-none")
	at ../../userprog/process.c:32
	`process_execute (const char *file_name)`
	#1  0xc002025e in run_task (argv=0xc0034a0c <argv+12>) at ../../thread
	s/init.c:288
	`process_wait (process_execute (task));`
	#2  0xc00208e4 in run_actions (argv=0xc0034a0c <argv+12>) at ../../thr
	eads/init.c:340
	`a->function (argv);`
	#3  main () at ../../threads/init.c:133
	`run_actions (argv);`

3. The name of the current thread is **args-none** with address **0xc010a000**.

	**struct threads of main**
	0xc000e000 {tid = 1, status = THREAD_BLOCKED,
	name = "main", '\000' <repeats 11 times>,
	stack = 0xc000eebc "\001", priority = 31,
	allelem = {prev = 0xc0034b50 <all_list>, next = 0xc0104020},
	elem = {prev = 0xc0036554 <temporary+4>, next = 0xc003655c <temporary+12>},
	pagedir = 0x0, magic = 3446325067}

	**struct threads of idle**
	0xc0104000 {tid = 2, status = THREAD_BLOCKED,
	name = "idle", '\000' <repeats 11 times>,
	stack = 0xc0104f34 "", priority = 0,
	allelem = {prev = 0xc000e020, next = 0xc010a020},
	elem = {prev = 0xc0034b60 <ready_list>, next = 0xc0034b68 <ready_list+8>},
	pagedir = 0x0, magic = 3446325067}

	**struct threads of args-none**
	0xc010a000 {tid = 3, status = THREAD_RUNNING,
	name = "args-none\000\000\000\000\000\000",
	stack = 0xc010afd4 "", priority = 31,
	allelem = {prev = 0xc0104020, next = 0xc0034b58 <all_list+8>},
	elem = {prev = 0xc0034b60 <ready_list>, next = 0xc0034b68 <ready_list+8>},
	pagedir = 0x0, magic = 3446325067}

4. The thread running **start_process**, **args-none**, is created in line 45
   of **userprog/process.c**.
   `tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);`

5. **0x0804870c** caused the page fault.

6. ```#0  _start (argc=<error reading variable: can't compute CFA for this frame>,
	           argv=<error reading variable: can't compute CFA for this frame>)
	   at ../../lib/user/entry.c:9```

7. The program is crashing because when we create a thread to run the user
   program, we pass in a variable to it. In the case of `args-none`,
   **args-none** is passed in as an argument to the user program. However,
   this argument has an address of 0xc0007d50. Thus, when the user program
   tries to access this argument, it will cause a page fault, since the address
   exceeds 0xC0000000 which is the upper bound for the user program virtual
   memory addresses.

## Additional Questions

### 1. Please identify a test case that uses an invalid stack pointer ($esp) when making a syscall. Provide the name of the test and explain how the test works.

The sc-bad-sp test uses a stack pointer that is invalid because it points to a bad address. The test first assigns (on line 18) the stack pointer to the address $.-(64*1024*1024) by moving the address into the contents of $esp, the stack pointer. This is a bad address because it is 64MB below the code segment. After setting an invalid stack pointer, the test makes a syscall (int  $0x30). We do all this in assembly code by putting these instructions as arguments to asm volatile. Because the syscall was made with an invalid stack pointer, it should terminate with a -1 exit code. If this test fails (and we do not exit), then the code will not exit and will reach the next line of the test which will tell us that we failed the test and log it.

### 2. Please identify a test case that uses a valid stack pointer when making a syscall, but the stack pointer is too close to a page boundary, so some of the syscall arguments are located in invalid memory

The sc-bad-arg test uses a valid stack pointer set at the very top of the stack - the user stack ends at 0xc0000000 and we have our pointer at 0xbffffffc, leaving 4 bytes of space left in the user stack. The next instruction (2nd instruction in the assembly code of line 14) moves SYS_EXIT to the location of our stack pointer %0 corresponds to the input operand which in this case is the integer specified SYS_EXIT. The last instruction on the same line invokes the system call designated by our stack pointer's location, which is the  SYS_EXIT. Since our stack pointer was at the top of the user stack with 4 bytes of it left, and SYS_EXIT takes 4 bytes of space, we have used all the space at the top of the user stack. We now go up the stack to look for the syscall arguments but are unable because the SYS_EXIT is at the very top and if we try to go further we'd be going into an invalid address. We therefore can't get the arguments and the process must be terminated with -1 exit code because the argument to the system call would be above the top of the user address space in invalid memory.


## 3. Identify one part of the project requirements which is not fully tested by the existing test suite. Explain what kind of test needs to be added to the test suite, in order to provide coverage for that part of the project.

in section3.1.7 in the specs, it is specified that "As part of a system call, the kernel must often access memory through pointers provided by a user
program. The kernel must be very careful about doing so, because the user can pass a null pointer." We don't yet have a test that checks that a syscall should fail if given a null pointer as the stack pointer. We can create a test similar to sc-bad-sp where in assembly code (using asm volatile), we assign the stack pointer to be a null pointer and then attempt to make a syscall. The correct result would be that the syscall would fail and we terminate with a -1 exit code. If the test is failed, the syscall won't exit, and we can therefore put fail ("should have called exit(-1)"); on the next line denoting that the test was failed.
