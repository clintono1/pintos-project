Design Document for Project 1: Threads
======================================

## Group Members

* Aaron Xu <aaxu@berkeley.edu>
* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>
* FirstName LastName <email@domain.example>


# Part 1: Efficient Alarm Clock

## Data structures and functions

Edit the thread data structure so it has an instance variable that
keeps track of whether it is sleeping or not.

```
struct thread
  {
    ...
    int ticks_to_sleep = 0;
    ...
  };
```

## Algorithms

First, when a thread calls `timer_sleep`, we can set its `ticks_to_sleep`
instance variable to the amount of ticks that it should sleep. Afterward,
we can block the thread with `thread_block()`. This would then automatically
block the thread and schedule another one to run. We can then edit
`timer_interrupt` so that it iterates through `all_list`, decrements the timer
of any sleeping threads, and unblock the thread if the timer is 0. This should
put the thread back onto the ready_list, so it will no longer be sleeping.


## Synchronization

## Rationale

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