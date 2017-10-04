/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "lib/kernel/list.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value)
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  struct thread *t = thread_current ();
  while (sema->value == 0)
    {
      list_push_back (&sema->waiters, &t->elem);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema)
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0)
    {
      sema->value--;
      success = true;
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up the highest priority thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (!list_empty (&sema->waiters)) {
    list_less_func *comparison = &compare_threads;
    struct list_elem *maxElem = list_max (&sema->waiters,
                                           comparison,
                                           NULL);
    struct thread *highest_priority_thread = list_entry (maxElem, struct thread, elem);
    list_remove (maxElem);
    thread_unblock (highest_priority_thread);
  }
  sema->value++;
  thread_yields_to_highest ();
  intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf () to see
   what's going on. */
void
sema_self_test (void)
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++)
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test (). */
static void
sema_test_helper (void *sema_)
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++)
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  struct thread *current = thread_current ();
  current->waiting_for_this_lock = lock;

  // Give donation when waiting for a lock
  enum intr_level old_level = intr_disable ();
  donate_to_holder (current);
  sema_down (&lock->semaphore);
  current->waiting_for_this_lock = NULL;
  lock->holder = current;

  /* Adds the lock to the threads list of locks when it acquires it */
  list_push_back (&current->locks, &lock->list_elem);

  // Set thread priority and accept donations.
  accept_from_waiters (current);
  thread_yields_to_highest ();
  intr_set_level (old_level);

}

/* Makes the current thread donate to the current holder of
   whatever lock it is waiting for. This also implements chain donation. */
void
donate_to_holder (struct thread *cur_thread) {
  /* Chain donation */
  // TODO: Check, maybe use priority lock instead of disable interrupts?
  struct thread *t = cur_thread;
  while (t->waiting_for_this_lock && t->waiting_for_this_lock->holder) {
    t = t->waiting_for_this_lock->holder;
    t->priority = MAX (t->priority, cur_thread->priority);
  }
}

/* Changes a threads priority to the max of the lock's waiters' priority.
   This is to cover the case where a lock is passed to another thread
   and the new holder needs its priority updated. It also allows a thread
   to update its priority after releasing a lock. */
void
accept_from_waiters (struct thread *t) {
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  int max_priority = t->base_priority;
  list_less_func *comparison = &compare_threads;
  for (e = list_begin (&t->locks); e != list_end (&t->locks);
       e = list_next (e))
    {
      struct lock *lock = list_entry (e, struct lock, list_elem);
      struct thread *highest_priority_thread = get_thread_with_most_priority (&lock->semaphore.waiters);
      max_priority = MAX (max_priority, highest_priority_thread->priority);
    }
  t->priority = max_priority;
}

// Used in list_max to compare waiters' priorities
bool compare_threads (const struct list_elem *a,
                      const struct list_elem *b,
                      void *aux UNUSED) {
  struct thread *t1 = list_entry (a, struct thread, elem);
  struct thread *t2 = list_entry (b, struct thread, elem);
  return t1->priority < t2->priority;
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success) {
    struct thread *current = thread_current ();
    lock->holder = current;
    list_push_back (&current->locks, &lock->list_elem);
    enum intr_level old_level = intr_disable ();
    accept_from_waiters (current);
    thread_yields_to_highest ();
    intr_set_level (old_level);
  }
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  lock->holder = NULL;

  /* Take lock out of thread's locks list, and updates the
     thread's priority. */
  struct thread *current = thread_current ();
  list_remove (&lock->list_elem);

  enum intr_level old_level = intr_disable ();
  accept_from_waiters (current);

  sema_up (&lock->semaphore);
  
  thread_yields_to_highest ();
  intr_set_level (old_level);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock)
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
  };

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock)
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  sema_init (&waiter.semaphore, 0);
  list_push_back (&cond->waiters, &waiter.elem);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED)
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) {
    int max_priority = -1;
    struct semaphore_elem *semaphore_elem;
    struct semaphore_elem *max_semaphore_elem;
    struct list_elem *e;
    enum intr_level old_level = intr_disable ();
    for (e = list_begin (&cond->waiters); e != list_end (&cond->waiters);
         e = list_next (e))
      {
        semaphore_elem = list_entry (e, struct semaphore_elem, elem);
        struct thread *highest_priority_thread = get_thread_with_most_priority (&semaphore_elem->semaphore.waiters);
        if (highest_priority_thread->priority > max_priority) {
          max_priority = highest_priority_thread->priority;
          max_semaphore_elem = semaphore_elem;
        }
      }
    list_remove (&max_semaphore_elem->elem);
    intr_set_level (old_level);
    sema_up (&max_semaphore_elem->semaphore);
  }
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock)
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}
