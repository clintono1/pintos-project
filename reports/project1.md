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

In our initial implementation, we did not store the original priority, so we could not restore its priority after its donation is done. Now we have an instance variable for each thread that keeps track of its original priority (`base_priority`).

Our initial implementation did not correctly account for multiple/nested donations. To counter this, we decided to utilize the `waiters` list that is an instance variable for each semaphore. Since each lock uses a semaphore, this kept track of all the threads waiting for a lock. Every time a lock is acquired, that thread goes through all the waiters of that lock and looks at their priorities in `accept_from_waiters`. The thread would find the highest and make that its new effective priority if it's higher than its old effective priority. This takes care of multiple/nested donations.

We also made it so that every time Thread A tries to acquire a lock, we see if that lock has a holder B, and if it does, we will update B's priority to be A's if A's is higher in `donate_to_holder`. This takes care of chain donations as well because we will be looking at B's effective priority, which already includes the donations it has received. Then when B releases that lock, in `accept_from_waiters`, we go through all the other locks B has, if any, and change B's priority to be the highest out of all the waiters of all the locks. If the highest priority is less than B's original priority or there aren't any waiters, we change B's priority to be its original priority.

We made sure to choose the highest priority thread for the other synchronization primitives as well. When we up a semaphore, we call `thread_yields_to_highest()`, which checks to see if the current thread is the highest priority, and if it isn't then it yields. This makes sure that the thread with the highest priority runs instead of the previously running thread. For condition variables, we signal to the highest priority thread so we unblock just that one. We do this by looking at all the semaphores waiting for that condition variable and signaling to the one that has the highest priority waiter.

By doing all the above, we make sure that we are always yielding to the highest priority thread immediately after any change in priority.


## Part 3: Multi-level Feedback Queue Scheduler (MLFQS)

Rather than using a priority queue to maintain all the ready threads ordered by their priorities, we found it to be more efficient to simply always be working with the `ready_list`. Using this implementation, we are able to efficiently access the thread with the highest priority when necessary by iterating through the `ready_list` rather than having to sort the priority queue every time we update the priority of the threads.

On the creation of every thread in `init_thread`, we set the `nice` value, the `t_recent_cpu` (which is 0 if the thread is `main`, the initial thread, or the value of the parent thread (`thread_current ()`), otherwise), and the `priority` by calling a function we defined called `update_mlfqs_priority(thread)`. When setting priorities, we are careful to make a distinction on how we set the priority based on the value of the `thread_mlfqs`. - We set `thread_set_priority` to only work if `thread_mlfqs` is false. We also check the tag in `timer_interrupt()` and `init_thread()` in order to set variables specific to mlfqs.

In `timer_interrupt` we call `thread_get_recent_cpu` on every tick, where the current thread's `t_recent_cpu` variable is incremented by one at every tick, and the `t_recent_cpu` of every other thread is updated according to the given formula at every second (more specifically at `timer_ticks() % TIMER_FREQ == 0`). the `load_avg` static variable is also updated at every second.



# Project Reflection

## Work Distribution

We all got together and brainstormed how to implement the different tasks. We also worked together via pair programming to write all the code and implement the tasks. Listed below, are the specific actions each member of our group did.

Aaron Xu - Wrote the documentation for Task 1 and implemented it. He also worked on the documentation and implementation of Task 2.

Aidan Meredith - Answered the additional questions of the design document. He also worked on the implementation of Task 3.

Louise Feng - Worked on the documentation of Task 2 and Task 3. She also worked on the implementation of Task 3 and wrote the final report for Task 3.

Yena Kim - Worked on the documentation of Task 2 and Task 3 and worked on the implementation of Task 2. She also wrote the final report for Tasks 1 and 2.


## Things That Went Well

Getting together in person to talk about how to approach and implement the different tasks was very helpful. Pair programming also went really well. We got into pairs to work on the different tasks. Originally, we tried assigning different parts to each member, but that proved to be really difficult to implement and debug. Working together allowed for more efficient coding and less errors and debugging. Although the bulk of the work was done in pairs, we completed the project working together as a whole; the whole project was done with everyone present, which allowed for the support and ideas of everyone in the group whenever there were issues or when something was unclear to someone. Additionally, being able to communicate our thoughts and ideas out loud contributed to a more organized workflow that kept us on task and with clear steps to take as we progressed. We were able to map out exactly what we wanted to do (with the help of what we had already learned while completing the design doc and the feedback from the design review) and were able to determine clearly how to approach each task. This also allowed us to write logical and readable code that was easy to follow and debug when necessary.


## Things That Could've Been Improved

The design document could have been better. We weren't sure what level of detail was required for the design document, and so we did not put as much information as was needed. We also were pretty unclear about synchronization primitives in general and did not grasp the concepts that well, so we could have read up more about the subject. In future design docs, we plan to have much more definite and clear documentation that outline much more of the steps we'd take in our code rather than just convey a surface understanding of each task. Specifically, as was brought up during the design review, our Data Structures and Functions portion will be a lot more code heavy and show mor precisely what we want to use and how we plan to use them. Another aspect that we could have handled better was agreeing on when we could meet up and work. We all have differenet schedules and had a difficult time communicting a good common time frame to work on the project. For future reference we should map out ahead of time all the time slots throughout the duration of the project everyone is available and commit to those times early rather than communicating on a day-by-day basis trying to determine when everyone is free.