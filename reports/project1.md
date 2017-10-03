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