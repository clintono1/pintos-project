Final Report for Project 1: Threads
===================================

# Changes Since Initial Design Document

## Part 1: Efficient Alarm Clock

We initially checked to see if a thread needs to be woken up by adding the 
instance variable `ticks_to_sleep` that keeps track of how many more ticks it 
will sleep. Every time there is a timer interrupt, we would then decrement the 
number of ticks of each sleeping thread and wake any that have a nonpositive 
value for `ticks_to_sleep`. However, we realized that this is an expensive way
to implement this since we would have to get, modify, and assign a variable for
every sleeping thread. Instead, we decided to set a wakeup time for each thread
when it is put to sleep, so that at each timer interrupt we just have to check
the value and compare it to the current time. If it is past the wakeup time then
we wake up that thread.


## Part 2: Priority Scheduler

In our initial implementation, we did not store the original priority, so we could not restore its priority after its donation is done. Now we have an instance variable for each thread that keeps track of its original priority.

Our initial implementation did not correctly account for multiple/nested
donations. To counter this, we decided to add an instance variable to each 
thread that kept track of the lock it is waiting for, if any. Then if we donate 
to a thread and see that it is waiting for a lock, we can also donate to that 
lock's holder and so on recursively. This will take care of chain donations.

Every time Thread A tries to acquire a lock, we see if that lock has a holder B, and if it does, we will update B's priority to be A's if A's is higher. Then when B releases that lock, we go through all the other locks B has, if any, and change B's priority to be the highest out of all the waiters of all the locks. If the highest priority is less than B's original priority or there aren't any watiers, we change B's priority to be its original priority.





## Part 3: Multi-level Feedback Queue Scheduler (MLFQS)

We In the design document, we did not go into detail about 






## Data structures and functions

We will need to keep track of a static variable `load_avg`. Then we can keep
track of another static variable `recent_cpu_coeff` which uses `load_avg` and
is used in the `recent_cpu` formula.

load_avg, recent_cpu_coeff, recent_cpu are updated once per second
for running thread only: increment recent_cpu by one every tick

Note that we compute these above variables based on the formulas given in the
project specs.

We also will need a static variable `ready_threads_pq` pointing to a priority queue
to hold all the ready threads in order of priority.

gotta make priority queue ahhh
linked list in order of priority
every time we update priority (every 4 ticks) we change the order of the list
use list_sort
every 4 ticks we update priority and check if sorted, if not we sort

if thread_mlfqs is true we dont do any priority donation so we check this boolean everywhere we do donation stuff


TODO:
implement set nice - need to figure out a way to update priority.
add recent_cpu instance variable
recent_cpu_coeff and load_avg are static variables
implement get recent cpu and load avg functions
ignore priority argument when a thread is created if thread_mlfqs is true then we set the priority
ignore thread_set_priority (since we set it)
thread_get_priority should return the threads current priority


DONE:
added nice instance variable
when initializing thread, get current thread (parent thread) and makes the new thread's niceness factor be equal to the parent's nice variable (if thread_mlfqs)
if there is no running thread (is this even possible???) set it to 0
not sure how this works for initializing the starting idle thread
implemented thread_get_nice (it just returns the current thread's nice variable)
created a new function that compares two threads' priority
in thread_set_nice i update the niceness factor and then will need to update priority
after priority is updated, it is compared to the max of the priorities and if it is less, then it will yield
added temporary static variable priority_queue just so the file compiles






use fixed point arithmetic for all calculations (pintos/src/threads/fixed-point.h)





## Algorithms

We keep track of all the ready threads and their priorities in one priority queue.
We update all the priorities (not including the threads that are blocked). We do this 
every 4th tick in `timer_interrupt()` by calling `thread_get_recentcpu()` and 
`thread_get_load_avg()` and returning the value of the formula given in the specs. This 
type of scheduling only occurs when the boolean variable `thread_mlfqs` is `true`, 
otherwise we use the scheduler implemented in part 2. When we recalculate the priorities, 
the order of the priority queue automatically changes. We update `load_avg`, `recent_cpu_coeff`
and `recent_cpu_coeff` every second in the same `timer_interrupt()` function. When we 
calculate `recent_cpu`, we are sure to calculate `recent_cpu_coeff` first in order to avoid 
overflow problems, then multiple it by `recent_cpu` in the formula.

## Synchronization

There should be no synchronization issues for this part of the project. This is because 
every time we access the global variables (`load_avg`, `recent_cpu_coeff`, and `ready_threads_pq`),
we are in the middle of an interrupt, so no other threads can access them at the same time.	
Since we are accessing shared data in only `timer_interrupt()` we do not have to worry about
race conditions.

## Rationale

The formulas for calculating each priority were given and we're simply putting
it to code. We chose to use one priority queue over 64 queues because the order will 
still remain the same. This reduces the space complexity by eliminating the empty queues.
However, by doing this, we have a very long priority queue, and there may be a better solution
to how we store the ready threads.