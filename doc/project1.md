Design Document for Project 1: Threads
======================================

## Group Members

* Aaron Xu <aaxu@berkeley.edu>
* Yena Kim <yenakim@berkeley.edu>
* Aidan Meredith <aidan_meredith@berkeley.edu>
* FirstName LastName <email@domain.example>


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

## Algorithms

## Synchronization

## Rationale

# Part 3: Multi-level Feedback Queue Scheduler (MLFQS)

## Data structures and functions

We will need to keep track of a static variable `load_avg`. Then we can keep
track of another static variable `recent_cpu_coeff` which uses `load_avg` and
is used in the `recent_cpu` formula.

Note that we compute these above variables based on the formulas given in the
project specs.

## Algorithms

We can calculate the priority of each thread in the priority queues (do not
update threads that are blocked). We do this every 4th tick in `timer_interrupt`.
We do this by calling `thread_get_recentcpu()` and `thread_get_load_avg()` and
returning the value of the formula given in the specs. We do this only if the
boolean variable `thread_mlfqs` is `true`, otherwise we use the scheduler
implemented in part 2. When we recalculate the priorities, we now have to place
the threads in the appropriate queues based on their new priorities. We iterate
through all the threads again to place them in the corresponding queue.

## Synchronization

## Rationale
The formulas for calculating each priority were given and we're simply putting
it to code.

