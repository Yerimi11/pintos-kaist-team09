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

bool thread_compare_priority (struct list_elem *l, struct list_elem *s, void *aux UNUSED);

/* Donate 추가 */
bool thread_compare_donate_priority (const struct list_elem *l, const struct list_elem *s, void *aux UNUSED);

/* PROJECT1: THREADS - Priority Scheduling */

bool sema_compare_priority (const struct list_elem *l, const struct list_elem *s, void *aux UNUSED){
	struct semaphore_elem *l_sema = list_entry (l, struct semaphore_elem, elem);
	struct semaphore_elem *s_sema = list_entry (s, struct semaphore_elem, elem);

	struct list *waiter_l_sema = &(l_sema->semaphore.waiters);
	struct list *waiter_s_sema = &(s_sema->semaphore.waiters);

	return list_entry (list_begin (waiter_l_sema), struct thread, elem)->priority
		 > list_entry (list_begin (waiter_s_sema), struct thread, elem)->priority;
}

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
   decrement it.

   - up or "V": increment the value (and wake up one waiting
   thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) {
	ASSERT (sema != NULL);

	sema->value = value;
	list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. This is
   sema_down function. */

/* Priority Scheduling 수정 */
void
sema_down (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);
	ASSERT (!intr_context ());

	old_level = intr_disable ();
	while (sema->value == 0) {
		// list_push_back (&sema->waiters, &thread_current ()->elem); // 주석 처리
		list_insert_ordered (&sema->waiters, &thread_current ()->elem, thread_compare_priority, 0); // waiter에 스레드를 넣어줄 때 우선순위를 고려하여 넣을 수 있도록
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
sema_try_down (struct semaphore *sema) {
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
   and wakes up one thread of those waiting for SEMA, if any.
   This function may be called from an interrupt handler. 
   세마포어 가동 또는 "V" 연산.  SEMA의 값 증가
   SEMA를 기다리는 사람들의 실타래가 있다면 깨워줍니다.
   이 함수는 인터럽트 핸들러에서 호출할 수 있다. */

/* sema_up( ) 함수의 경우, waiters 리스트에 있는 동안 우선순위의 변동이 있을 수 있다.
	따라서, thread_unblock( ) 함수를 호출하기 전에 해당 리스트를 내림차순으로 정렬할 수 있도록 한다.
	그런데 unblock 된 스레드가 현재 CPU를 점유하고 있는 스레드보다 우선순위가 높을 수 있다.
	thread_test_preemption( ) 함수를 실행하여 CPU를 점유할 수 있도록 한다. */

/* Priority Scheduling 수정 */
void
sema_up (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);

	old_level = intr_disable ();

	/* ----------- project1 ------------ */
	if (!list_empty (&sema->waiters)){
		list_sort(&sema->waiters, &thread_priority_compare, 0);
		thread_unblock (list_entry (list_pop_front (&sema->waiters),
					struct thread, elem));
	}
	/* --------------------------------- */

	sema->value++;

	/* ----------- project1 ------------ */
	if (preempt_by_priority()){
		if (intr_context()) {
			intr_yield_on_return();
		} else {
			thread_yield();
		}
	}
	intr_set_level (old_level);
	/* --------------------------------- */
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) {
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

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) {
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
lock_init (struct lock *lock) {
	ASSERT (lock != NULL);

	lock->holder = NULL;
	sema_init (&lock->semaphore, 1); // 1로 초기화 -> xmutex로 사용하겠다는 이야기
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (!lock_held_by_current_thread (lock));


	/* ----------- Project 1 ------------ */
	struct thread *curr = thread_current();
	
	if (lock->holder) {
		curr->wait_on_lock = lock;
		list_push_back(&lock->holder->donation_list, &curr->donation_elem);
		donate_priority();
	}
	/* ---------------------------------- */
	
	sema_down (&lock->semaphore);

	/* ----------- Project 1 ------------ */
	curr->wait_on_lock = NULL;
	lock->holder = curr;
	/* ---------------------------------- */
}

/* Tries to acquires LOCK and returns true if successful or false on failure.
   The lock must not already be held by the current thread.

   This function will not sleep, so it may be called within an interrupt handler. */
bool
lock_try_acquire (struct lock *lock) {
	bool success;

	ASSERT (lock != NULL);
	ASSERT (!lock_held_by_current_thread (lock));

	success = sema_try_down (&lock->semaphore);
	if (success)
		lock->holder = thread_current ();
	return success;
}

/* Releases LOCK, which must be owned by the current thread.
   This is lock_release function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) {
	ASSERT (lock != NULL);
	ASSERT (lock_held_by_current_thread (lock));

	/* ----------- Project 1 ------------ */
	remove_donation_list_elem(lock);
	reset_priority();
	/* ---------------------------------- */

	lock->holder = NULL;
	sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) {
	ASSERT (lock != NULL);

	return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem {
	struct list_elem elem;              /* List element. */
	struct semaphore semaphore;         /* This semaphore. */
};

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond) {
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
cond_wait (struct condition *cond, struct lock *lock) {
	struct semaphore_elem waiter;

	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	sema_init (&waiter.semaphore, 0);
	// list_push_back (&cond->waiters, &waiter.elem);
	list_insert_ordered (&cond->waiters, &waiter.elem, sema_compare_priority, 0);

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
cond_signal (struct condition *cond, struct lock *lock UNUSED) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	/* ----------- Project 1 ------------ */
	if (!list_empty (&cond->waiters))
		list_sort(&cond->waiters, &sema_priority_compare, NULL);
		sema_up (&list_entry (list_pop_front (&cond->waiters),
					struct semaphore_elem, elem)->semaphore);
	/* ---------------------------------- */
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);

	while (!list_empty (&cond->waiters))
		cond_signal (cond, lock);
}

/* ------------ project 1 ------------ */
/* compare thread priority value in condition variable
  1. make semaphore_elem by elem *a and *b
  2. get semaphore's waiters by semaphore_elem
  3. get thread priority of first thread of semaphore's waiters 
  compare and return
 */
static bool sema_priority_compare(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED){
	struct semaphore_elem *sema_elem_a;
	struct semaphore_elem *sema_elem_b;
	struct list *a_waiter;
	struct list *b_waiter;
	int a_priority;
	int b_priority;

	sema_elem_a = list_entry(a, struct semaphore_elem, elem);
	sema_elem_b = list_entry(b, struct semaphore_elem, elem);

	a_waiter = &(sema_elem_a->semaphore.waiters);
	b_waiter = &(sema_elem_b->semaphore.waiters);
	
	a_priority = list_entry(list_front(a_waiter), struct thread, elem)->priority;
	b_priority = list_entry(list_front(b_waiter), struct thread, elem)->priority;

	if (a_priority > b_priority){
		return true;
	}else{
		return false;
	}
}

/* if current thread want to acqurie lock and there is lock holder,
	donate current thread priority to every single thread that lock holder is waiting for 
	( depth limit = 8 according to test case ) */
void donate_priority(void) {
	int depth;
	struct thread* curr = thread_current();
	struct thread* holder = curr->wait_on_lock->holder;

	for (depth = 0; depth < 8; depth++) {
		if (!curr->wait_on_lock) break;
		holder = curr->wait_on_lock->holder;
		holder->priority = curr->priority;
		curr = holder;
	}
}

/* when current thread release THIS LOCK(argument struct lock *lock),
 the current thread remove only threads which want THIS LOCK in the current thread's donation_list.
 In which if current thread's donation list is not empty, it means current thread holds another lock. */
void remove_donation_list_elem(struct lock *lock){
	struct thread *curr = thread_current();

	if (!list_empty(&curr->donation_list)) {
		struct list_elem *e = list_begin(&curr->donation_list);
		
		for (e; e != list_end(&curr->donation_list); e = list_next(e)){
			struct thread *t = list_entry(e, struct thread, donation_elem);
			if (lock == t->wait_on_lock){
				list_remove(&t->donation_elem);
			}
		}
	}
}

/* reset current thread priority.
	if there is donated thread to currnet thread, find the bigger priority and set to current thread priority.
	if not, set current priority to initial priority.*/
void reset_priority(void){
	struct thread *curr = thread_current();
	curr->priority = curr->initial_priority;

	if (!list_empty(&curr->donation_list)){
		list_sort(&curr->donation_list, &thread_donate_priority_compare, NULL);
		
		struct list_elem *donated_e = list_front(&curr->donation_list);

		int max_donated_priority = list_entry(donated_e, struct thread, donation_elem)->priority;
		if (curr->priority<max_donated_priority){
			curr->priority = max_donated_priority;
		}
	}
}
/* ------------------- project 1 functions end ------------------------------- */