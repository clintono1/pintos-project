Design Document for Project 2: User Programs
============================================

## Group Members

* Aaron Xu <aaxu@berkeley.edu>
* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>

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