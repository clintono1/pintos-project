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

In our initial implementation, we did not store the base priority of the thread,
so we could not restore its priority after its donation is done. Now we have an
instance variable (`base_priority`) in the thread struct that tracks this.

Rather than using a priority queue data structure, we found it to be more
efficient to just search for the thread with the highest priority when we call
`thread_yield` or any time we want to switch threads. Using this implementation,
we are able to efficiently access the thread with the highest priority when
necessary by iterating through the `ready_list` rather than having to sort the
priority queue every time we update the priority of the threads. We figured that
because the thread priorities were always changing, it would be better to
iterate through and find the max every time in `O(n)` runtime than sort every
update which is `O(log(n))`.

Our initial implementation did not correctly account for multiple/nested
donations. To counter this, we decided to utilize the `waiters` list that is
a member of the semaphore struct. Since each lock uses a semaphore, we are able
to use this list to keep track of all the threads waiting for a particular lock.
Every time a lock is acquired, that thread goes through all the waiters of that
lock and sets its own priority to the highest of its own or any of its waiter's
priorities. If a new thread waits on a lock, we have it try to recursively
donate its priority to the holder of that lock, and if that thread is also
waiting on a lock, we can recursively donate until we reach a thread that is not
waiting on a lock. This is accomplished in the function `donate_to_holder`.

We made sure to choose the highest priority thread for the other synchronization
primitives as well. When we up a semaphore, we call `thread_yield`. This works
because we changed `next_thread_to_run` to return the thread with the highest
priority. This makes sure that the thread with the highest priority runs instead
of the previously running thread. For condition variables, rather than sending
a signal to any thread, we send it to the one with the highest priority. We do
this by looking at all the waiters of all the semaphores of that condition
variable and signal that one to wake up. We also call `thread_yield` in all cases
in which a thread's priority changes, (eg. when a thread acquires a lock,
releases a lock, sets its own priority).

By doing all the above, we ensure that we are always yielding to the
highest priority thread immediately after any change in priority.


## Part 3: Multi-level Feedback Queue Scheduler (MLFQS)

Our implementation for this is the same as for the Priority Queue scheduler.
We still keep using the `ready_list` and manually search through for the highest
priority thread to run.

On the creation of every thread in `init_thread`, we set the `nice` value, the
`t_recent_cpu`, which is 0 if it is the initial thread ('main'), or the value of
the parent thread otherwise. We calculate and update the priority of a thread
by calling `update_mlfqs_priority`. When setting priorities, we are careful to
make a distinction on how we set the priority based on the value of the
`thread_mlfqs`. - We set `thread_set_priority` to only work if `thread_mlfqs`
is false. We also check the tag in `timer_interrupt()` and `init_thread()` in
order to set variables specific to mlfqs.

In `timer_interrupt` we call `thread_set_recent_cpu` on every tick, where the
current thread's `t_recent_cpu` variable is incremented by one at every tick,
and the `t_recent_cpu` of every other thread is updated according to the given
formula at every second (specifically at `timer_ticks() % TIMER_FREQ == 0`).
The `load_avg` static variable is also updated at every second by the function
`thread_set_load_avg`.



# Project Reflection

## Work Distribution

We all got together and brainstormed how to implement the different tasks.
We also worked together via pair programming to write all the code and implement
the tasks. Listed below, are the specific actions each member of our group did.

#### Aaron Xu
Documented and Implemented Part 1. Pair programmed with Yena to implement Part 2.
Also debugged and implemented part of Part 3 along with the rest of the group.

#### Aidan Meredith
Answered the additional questions of the design document and
pair programmed with Louise and worked with the rest of the group to finish Part 3.

#### Louise Feng
Worked on the documentation of Part 2 and pair programmed with
Aidan to implement Part 3. Also wrote the final report for Part 3.

#### Yena Kim
Paired programmed with Aaron on Part 2 and helped implemented Task 3.
Also wrote the final report for Tasks 1 and 2.

## Things That Went Well

Getting together in person to talk about how to approach and implement the different tasks was very helpful. Pair programming also went really well. We got into pairs to work on the different tasks. Originally, we tried assigning different parts to each member, but that proved to be really difficult to implement and debug. Working together allowed for more efficient coding and less errors and debugging. Although the bulk of the work was done in pairs, we completed the project working together as a whole; the whole project was done with everyone present, which allowed for the support and ideas of everyone in the group whenever there were issues or when something was unclear to someone. Additionally, being able to communicate our thoughts and ideas out loud contributed to a more organized workflow that kept us on task and with clear steps to take as we progressed. We were able to map out exactly what we wanted to do (with the help of what we had already learned while completing the design doc and the feedback from the design review) and were able to determine clearly how to approach each task. This also allowed us to write logical and readable code that was easy to follow and debug when necessary.


## Things That Could've Been Improved

The design document could have been better. We weren't sure what level of detail was required for the design document, and so we did not put as much information as was needed. We also were pretty unclear about synchronization primitives in general and did not grasp the concepts that well, so we could have read up more about the subject. In future design docs, we plan to have much more definite and clear documentation that outline much more of the steps we'd take in our code rather than just convey a surface understanding of each task. Specifically, as was brought up during the design review, our Data Structures and Functions portion will be a lot more code heavy and show mor precisely what we want to use and how we plan to use them. Another aspect that we could have handled better was agreeing on when we could meet up and work. We all have differenet schedules and had a difficult time communicting a good common time frame to work on the project. For future reference we should map out ahead of time all the time slots throughout the duration of the project everyone is available and commit to those times early rather than communicating on a day-by-day basis trying to determine when everyone is free.