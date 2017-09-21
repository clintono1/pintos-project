Design Document for Project 1: Threads
======================================

## Group Members

* Aaron Xu <aaxu@berkeley.edu>
* Yena Kim <yenakim@berkeley.edu>
* Aidan Meredith <aidan_meredith@berkeley.edu>
* Louise Feng <louise.feng@berkeley.edu>


# Part 1: Efficient Alarm Clock

## Data structures and functions

Edit the thread data structure so it has an instance variable that
keeps track of whether it is sleeping or not. A `ticks_to_sleep` with value
**-1** means that that thread is not sleeping. This is so that we don't
accidentally unblock a thread that was not sleeping.

```
struct thread
  {
    ...
    int ticks_to_sleep = -1;
    ...
  };
```

We will also need a new static variable `block_list` that keeps track of
all the threads that are blocked.

`static struct list block_list;`


## Algorithms

First, when a thread calls `timer_sleep`, we can set its `ticks_to_sleep`
instance variable to the amount of ticks that it should sleep. Afterward,
we can block the thread with `thread_block()`. This would then automatically
block the thread and schedule another one to run. We can then edit
`timer_interrupt` so that it iterates through `block_list`, decrements the timer
of any sleeping threads. If the `ticks_to_sleep` becomes 0, set it to -1 and
unblock the thread. This should put the thread back onto the ready_list,
so it will no longer be sleeping.


## Synchronization

There should be no synchronization issues because we are not accessing any
shared resources. The only thing we are changing the thead's state, the
`block_list`, and the `ticks_to_sleep` of the threads. The running thread will
call `timer_sleep` which accesses its own `ticks_to_sleep`. The only other
function that should access this is the timer_interrupt, and that does not
provide a race condition. Then, we call `thread_block` which requires
interrupts to be disabled, so there will not be a race condition there either.

## Rationale

This is better than using a lock or a semaphore because that would require an
additional synchronization variable. We can directly use thread_block() so we
do not have to deal with the extra overhead of using a synchronization variable.
The only way that we could think of unblocking the sleeping thread was to
access the threads in `timer_interrupt()` through the `all_list` variabe.
However, this would require us to look through all the threads to determine
whether or not they were sleeping. It would be faster to keep a list that keeps
track of all the blocked threads.

# Part 2: Priority Scheduler

## Data structures and functions

We will edit the lock struct so that it now contains one more variable.

```
struct lock
  {
    ...
    int highestPriorityWaiting;
    ...
  };
```

We need this variable because we need to keep track of the highest priority thread
waiting and then change the holder of this lock to have this priority if it is higher.
We will also modify the `sema_down` function so that it sets the priority of the lock's holder
(if there is one), to a higher priority of the current thread has a
higher priority. We'll also edit the `next_thread_to_run` function so that
it returns the thread with the highest priority. We must also modify `timer_interrupt()`
so that we yield if our effective priority is no longer the highest.
Lastly, we'll need a new function `getEffectivePriority` that returns the
effective priority of the current thread.

## Algorithms

### Choosing the next thread to run
We can iterate through `all_list` and find the thread with the highest priority.
This will be implemented in `next_thread_to_run` by simply iterating through the list
and keeping track of the max priority thread and runs in O(n) time.

### Acquiring a lock:
When a function acquires a lock, it will call `lock_acquire`. At this point, it
should update the `highestPriorityWaiting` instance variable of the lock if
it is higher than the current value. The `lock_aquire` function calls `sema_down`,
so it should update the lock's donation properly.

### Releasing a Lock:
Before releasing the lock, we will set the `highestPriorityWaiting` variable of
the lock to 0. Then, we have to update the priority of the current thread
with its effective priority.

### Computing the Effective Priority:
Inside `getEffectivePriority`, we can iterate through all the locks and
see if the current thread is a holder of any of them. If so, we set the priority
of the thread to the maximum `highestPriorityWaiting` of all the threads.

### Priority Scheduling for Semaphores, Locks, and Condition Variables:
All of the steps above should apply to semaphores and since locks call
semaphore methods internally, it should apply to locks as well. The only thing
we would have to change for semaphores is that if a thread is waiting to down
a semaphore, all current holders of the semaphore should have their priority
raised to the highest priority waiter. In condition variables, we will
have to set the priority of the holder to match the waiter with the highest priority.

### Changing Thread's Priority:
The thread's priority should be changed at the beginning of `timer_interrupt()`.
It should get its priority changed to its effective priority.


## Synchronization
The only global variables we access are the the synchronization primatives. One
situation of a race condition happening is if two threads try to set
a synchronization primative's `highestPriorityWaiting` variable at the same time.
Then, it becomes possible that both threads see a value of 0 and try to set it to
their own priority and then the lower priority may end up as the final value. In order
to prevent this, we set this priority inside the `sema_down` function after interrupts
are disabled. This way, one thread cannot access the value until the other thread
is done modifying this value.


## Rationale
The reason we used this method was because the list struct was already defined for us,
and we weren't sure about the overhead or how other data structures would fit into
pintos. We considered saving a binary max heap to keep track of the thread with
the highest priority in order. However, this would make it so we would have to perform
a O(log(n)) operation every time we change the priority of a thread. We did not know
if it was worth this overhead to reduce the time it takes to find the next thread to run.


# Part 3: Multi-level Feedback Queue Scheduler (MLFQS)

## Data structures and functions

We will need to keep track of a static variable `load_avg`. Then we can keep
track of another static variable `recent_cpu_coeff` which uses `load_avg` and
is used in the `recent_cpu` formula.

Note that we compute these above variables based on the formulas given in the
project specs.

We also will need a static variable `ready_threads_pq` pointing to a priority queue
to hold all the ready threads in order of priority.

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



# Additional Questions

## 1. Priority Donations Not Taken into Account

## 2. MLFQS Scheduler Table

timer ticks | R(A) | R(B) | R(C) | P(A) | P(B) | P(C) | thread to run
------------|------|------|------|------|------|------|--------------
 0          | 0    | 0    | 0    | 63   | 61   | 59   | A
 4          | 4    | 0    | 0    | 62   | 61   | 59   | A
 8          | 8    | 0    | 0    | 61   | 61   | 59   | A + B
12          | 10   | 2    | 0    | 60   | 60   | 59   | A + B
16          | 12   | 4    | 0    | 60   | 60   | 59   | A + B
20          | 14   | 6    | 0    | 59   | 59   | 59   | A + B + C
24          | 15.33| 7.33 | 1.33 | 59   | 59   | 58   | A + B
28          | 17.33| 9.33 | 1.33 | 58   | 58   | 58   | A + B + C
32          | 18.66| 10.66| 2.66 | 58   | 58   | 58   | A + B + C
36          | 20   | 12   |  4   | 58   | 58   | 58   | A + B + C

## 3. Ambiguities in Scheduler Spec

When multiple threads have the same priority, it isn't clear which thread is running at time of timer tick (which would affect the recent_cpu value). In this case, we tried to imagine dividing these ticks into equal parts and incrementing recent_cpu by these values.  This is not specified in the spec so the program's intended behavior is unknown.
