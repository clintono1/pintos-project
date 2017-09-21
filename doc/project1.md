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
shared resources.

## Rationale

The reason we use this method is because we may need to

# Part 2: Priority Scheduler

## Data structures and functions

## Algorithms

## Synchronization

## Rationale

# Part 3: Multi-level Feedback Queue Scheduler (MLFQS)

## Data structures and functions

## Algorithms

## Synchronization

## Rationale
