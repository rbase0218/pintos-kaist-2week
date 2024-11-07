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

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
   decrement it.

   - up or "V": increment the value (and wake up one waiting
   thread, if any). */
/* 	세마포어 SEMA를 VALUE로 초기화합니다.
	세마포어는 음이 아닌 정수값과 이를 조작하는 두 개의 원자적 연산자로	구성됩니다:

	down 또는 "P": 값이 양수가 될 때까지 대기한 후,	해당 값을 감소시킵니다.
	up 또는 "V": 값을 증가시킵니다 (그리고 대기 중인 스레드가 있다면 그 중 하나를 깨웁니다). */
void sema_init(struct semaphore *sema, unsigned value)
{
	ASSERT(sema != NULL);

	sema->value = value;
	list_init(&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. This is
   sema_down function. */
/*	세마포어에 대한 down 또는 "P" 연산입니다. SEMA의 값이 양수가 될 때까지 대기한 후 원자적으로 이 값을 감소시킵니다.
	이 함수는 슬립 상태에 들어갈 수 있으므로 인터럽트 핸들러 내에서	호출되어서는 안 됩니다.
	이 함수는 인터럽트가 비활성화된	상태에서 호출될 수 있지만, 만약 슬립 상태에 들어간다면
	다음으로 스케줄된 스레드가 아마도 인터럽트를 다시 활성화할 것입니다. 이것이 sema_down 함수입니다. */
void sema_down(struct semaphore *sema)
{
	enum intr_level old_level;

	ASSERT(sema != NULL);
	ASSERT(!intr_context());

	old_level = intr_disable();
	while (sema->value == 0)
	{
		// list_push_back(&sema->waiters, &thread_current()->elem);
		list_insert_ordered(&sema->waiters, &thread_current()->elem, compare_priority, NULL);
		thread_block();
	}
	sema->value--;
	intr_set_level(old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
/* 세마포어에 대한 down 또는 "P" 연산이지만, 세마포어가 이미 0이 아닌
	경우에만 수행됩니다. 세마포어가 감소되면 true를,
	그렇지 않으면 false를 반환합니다.
	이 함수는 인터럽트 핸들러에서 호출될 수 있습니다. */
bool sema_try_down(struct semaphore *sema)
{
	enum intr_level old_level;
	bool success;

	ASSERT(sema != NULL);

	old_level = intr_disable();
	if (sema->value > 0)
	{
		sema->value--;
		success = true;
	}
	else
		success = false;
	intr_set_level(old_level);

	return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
/* 세마포어에 대한 up 또는 "V" 연산입니다. SEMA의 값을 증가시키고
	SEMA를 기다리는 스레드가 있다면 그 중 하나를 깨웁니다.
	이 함수는 인터럽트 핸들러에서 호출될 수 있습니다. */
void sema_up(struct semaphore *sema)
{
	enum intr_level old_level;

	ASSERT(sema != NULL);

	old_level = intr_disable();
	if (!list_empty(&sema->waiters))
		thread_unblock(list_entry(list_pop_front(&sema->waiters),
								  struct thread, elem));
	sema->value++;
	check_preempt();
	intr_set_level(old_level);
}

static void sema_test_helper(void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
between a pair of threads.  Insert calls to printf() to see
what's going on. /
/ 두 스레드 사이에서 제어권이 "핑퐁"처럼 오가는 세마포어에 대한
자체 테스트입니다. 진행 상황을 보려면 printf() 함수를
호출하여 확인할 수 있습니다. */
void sema_self_test(void)
{
	struct semaphore sema[2];
	int i;

	printf("Testing semaphores...");
	sema_init(&sema[0], 0);
	sema_init(&sema[1], 0);
	thread_create("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
	for (i = 0; i < 10; i++)
	{
		sema_up(&sema[0]);
		sema_down(&sema[1]);
	}
	printf("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper(void *sema_)
{
	struct semaphore *sema = sema_;
	int i;

	for (i = 0; i < 10; i++)
	{
		sema_down(&sema[0]);
		sema_up(&sema[1]);
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
/* LOCK을 초기화합니다. 락은 특정 시점에 최대 하나의 스레드만이 보유할 수 
   있습니다. 우리의 락은 "재귀적"이지 않습니다. 즉, 현재 락을 보유하고 있는 
   스레드가 같은 락을 다시 획득하려고 하는 것은 오류입니다.

   락은 초기값이 1인 세마포어의 특수한 형태입니다. 락과 세마포어의 차이점은 
   두 가지입니다. 첫째, 세마포어는 1보다 큰 값을 가질 수 있지만, 락은 한 번에 
   하나의 스레드만이 소유할 수 있습니다. 둘째, 세마포어는 소유자가 없어서 
   한 스레드가 세마포어를 "down"하고 다른 스레드가 "up"할 수 있지만, 락은 
   동일한 스레드가 획득과 해제를 모두 수행해야 합니다. 이러한 제약이 부담스러울 
   때는 락 대신 세마포어를 사용하는 것이 좋은 신호입니다. */
void lock_init(struct lock *lock)
{
    ASSERT(lock != NULL);
    lock->holder = NULL;
    sema_init(&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
/* LOCK을 획득하며, 필요한 경우 사용 가능해질 때까지 대기합니다. 
   현재 스레드가 이미 락을 보유하고 있으면 안 됩니다.

   이 함수는 대기 상태에 들어갈 수 있으므로 인터럽트 핸들러 내에서 
   호출되어서는 안 됩니다. 인터럽트가 비활성화된 상태에서 이 함수를 
   호출할 수 있지만, 대기가 필요한 경우 인터럽트가 다시 활성화됩니다. */
void lock_acquire(struct lock *lock)
{
	// Execpt Handler -> PASS
    ASSERT(lock != NULL);
    ASSERT(!intr_context());
    ASSERT(!lock_held_by_current_thread(lock));

	// [생각]
	// - 흭득하려는 Lock의 Handler에 이미 누군가 존재한다. -> Lock이 다른 스레드에서 보유되고 있다.
	// - 다음의 로직을 실행한다.
	// 	1. Current Thread가 기다리는 Lock 추가하기
	//  2. Handler의 기부자 목록에 현재 Thread 추가하기
	//  3. Handler의 Priority에게 Donation하기
	//   *해당 함수가 실행된다는 것은 현재 Thread보다 더 높은 우선순위를 보유했다는 것.
	//   *기부자 목록의 Front에 추가한다.
	// - 존재하지 않는다면, Wait_For_lock을 해제한다. -> 이전에 기다리는 것이 있었을 수 있음.

	struct thread* curr_thread = thread_current();
	if(lock->holder != NULL)
	{
		curr_thread->wait_target_lock = lock;
		list_push_front(&lock->holder->donors, &curr_thread->donor_elem);
		// list_insert_ordered(&lock->holder->donors, &curr_thread->donor_elem, compare_priority_donation, NULL);
		donation_priority();

		// Return을 하지 않는 이유
		// - sema_down 함수에서 Value가 0일때 처리가 되어있기 때문.
	}

	// Sema_Down에서 Value가 0이면, 현재 Thread를 Block으로 처리한다.
    sema_down(&lock->semaphore);
	curr_thread->wait_target_lock = NULL;
    lock->holder = curr_thread;
}

void donation_priority()
{
	// [생각]
	// - 우선순위를 lock이 없는 친구가 나올 때 까지 전달한다.
	// - 현재 내가 원하는 lock이 실행 중이 아닐 수 있다. 다른 Lock에 걸린 Holder가 쥐고 있을 수 있음.
	// * 해당 함수는 Wait에 들어가는 친구를 기준으로 사용할 수 있다.
	struct thread* curr_thread = thread_current();
	struct thread* holder = curr_thread->wait_target_lock->holder;

	while(holder != NULL)
	{
		// 우선순위 Holder에게 할당.
		holder->priority = curr_thread->priority;

		if(holder->wait_target_lock == NULL)
			holder = NULL;
		else
			holder = holder->wait_target_lock->holder;
	}	
}

void refresh_donation_priority()
{
	struct thread* curr_thread = thread_current();
	curr_thread->priority = curr_thread->origin_priority;

	if(!list_empty(&curr_thread->donors))
	{
		struct thread* max = list_entry(list_front(&curr_thread->donors), struct thread, donor_elem);
		if(max->priority > curr_thread->priority)
			curr_thread->priority = max->priority;
	}
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
/* LOCK의 획득을 시도하고 성공하면 true를, 실패하면 false를 반환합니다. 
   현재 스레드가 이미 락을 보유하고 있으면 안 됩니다.

   이 함수는 대기 상태에 들어가지 않으므로 인터럽트 핸들러 내에서 
   호출될 수 있습니다. */
bool lock_try_acquire(struct lock *lock)
{
    bool success;
    ASSERT(lock != NULL);
    ASSERT(!lock_held_by_current_thread(lock));
    success = sema_try_down(&lock->semaphore);
    if (success)
        lock->holder = thread_current();
    return success;
}

/* Releases LOCK, which must be owned by the current thread.
   This is lock_release function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
/* 현재 스레드가 소유한 LOCK을 해제합니다.
   이것이 lock_release 함수입니다.

   인터럽트 핸들러는 락을 획득할 수 없으므로, 인터럽트 핸들러 내에서 
   락을 해제하려고 하는 것은 의미가 없습니다. */
void lock_release(struct lock *lock)
{
	// Except Handler -> PASS
    ASSERT(lock != NULL);
    ASSERT(lock_held_by_current_thread(lock));

	// [생각]
	// *우선순위를 할당 받은 Thread가 Lock을 반환할 수 있기 때문에 이를 확인해야 한다.
	// *donors 목록을 순회하면서 찾고 있는 Lock이 반환 받은 Lock과 동일한 Element를 탐색한다.
	// *해당 목록을 List에서 제거한다.
	struct list_elem *e;
	struct thread* curr_thread = thread_current();
    
    for (e = list_begin(&curr_thread->donors); e != list_end(&curr_thread->donors); e = list_next(e))
    {
        struct thread *donor = list_entry(e, struct thread, donor_elem);
		if(donor->wait_target_lock == lock)
			list_remove(&donor->donor_elem);
    }
	refresh_donation_priority();

    lock->holder = NULL;
    sema_up(&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
/* 현재 스레드가 LOCK을 보유하고 있다면 true를, 그렇지 않으면 false를 
   반환합니다. (다른 스레드가 락을 보유하고 있는지 테스트하는 것은 
   경쟁 상태를 유발할 수 있다는 점에 주의하세요.) */
bool lock_held_by_current_thread(const struct lock *lock)
{
    ASSERT(lock != NULL);
    return lock->holder == thread_current();
}

/* One semaphore in a list. */
/* 리스트 내의 하나의 세마포어입니다. */
struct semaphore_elem
{
    struct list_elem elem;       /* List element. */  /* 리스트 요소입니다. */
    struct semaphore semaphore; /* This semaphore. */ /* 이 세마포어입니다. */
};

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
/* 조건 변수 COND를 초기화합니다. 조건 변수는 한 코드가 조건을 
   신호로 보내고 협력하는 코드가 그 신호를 받아 이에 따라 
   행동할 수 있게 합니다. */
void cond_init(struct condition *cond)
{
    ASSERT(cond != NULL);
    list_init(&cond->waiters);
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
	we need to sleep. /
	/ 원자적으로 LOCK을 해제하고 다른 코드에 의해 COND가 신호될 때까지
	대기합니다. COND 신호를 받은 후, 함수가 반환되기 전에 LOCK을
	다시 획득합니다. 이 함수를 호출하기 전에 LOCK이 보유되어
	있어야 합니다.
	이 함수로 구현된 모니터는 "Hoare" 방식이 아닌 "Mesa" 방식입니다.
	즉, 신호를 보내고 받는 것이 원자적 연산이 아닙니다. 따라서,
	일반적으로 호출자는 대기가 완료된 후 조건을 다시 확인해야 하며,
	필요한 경우 다시 대기해야 합니다.
	주어진 조건 변수는 단 하나의 락과만 연관되지만, 하나의 락은
	여러 개의 조건 변수와 연관될 수 있습니다. 즉, 락에서 조건
	변수로의 일대다 매핑이 존재합니다.
	이 함수는 슬립 상태에 들어갈 수 있으므로 인터럽트 핸들러 내에서
	호출되어서는 안 됩니다. 이 함수는 인터럽트가 비활성화된
	상태에서 호출될 수 있지만, 슬립이 필요한 경우 인터럽트가
	다시 활성화될 것입니다. */
void cond_wait(struct condition *cond, struct lock *lock)
{
	struct semaphore_elem waiter;

	ASSERT(cond != NULL);
	ASSERT(lock != NULL);
	ASSERT(!intr_context());
	ASSERT(lock_held_by_current_thread(lock));

	sema_init(&waiter.semaphore, 0);
	// list_push_back(&cond->waiters, &waiter.elem);
	list_insert_ordered(&cond->waiters, &waiter.elem, compare_priority_sema, NULL);
	lock_release(lock);
	sema_down(&waiter.semaphore);
	lock_acquire(lock);
}

bool compare_priority_sema(const struct list_elem* a, const struct list_elem* b, void* aux UNUSED)
{
    struct semaphore_elem *sema_a = list_entry(a, struct semaphore_elem, elem);
    struct semaphore_elem *sema_b = list_entry(b, struct semaphore_elem, elem);

	struct list *list_a = &sema_a->semaphore.waiters;
	struct list *list_b = &sema_b->semaphore.waiters;

	struct thread *el_a = list_entry(list_begin(list_a), struct thread, elem);
	struct thread *el_b = list_entry(list_begin(list_b), struct thread, elem);

	return el_a->priority > el_b->priority;
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_signal(struct condition *cond, struct lock *lock UNUSED)
{
	ASSERT(cond != NULL);
	ASSERT(lock != NULL);
	ASSERT(!intr_context());
	ASSERT(lock_held_by_current_thread(lock));

	if (!list_empty(&cond->waiters))
		list_sort(&cond->waiters, compare_priority_sema, NULL);
		sema_up(&list_entry(list_pop_front(&cond->waiters), struct semaphore_elem, elem)->semaphore);
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_broadcast(struct condition *cond, struct lock *lock)
{
	ASSERT(cond != NULL);
	ASSERT(lock != NULL);

	while (!list_empty(&cond->waiters))
		cond_signal(cond, lock);
}

bool compare_priority_donation(const struct list_elem* a, const struct list_elem* b, void* aux UNUSED)
{
	struct thread *el_a = list_entry(a, struct thread, donor_elem);
	struct thread *el_b = list_entry(b, struct thread, donor_elem);
	return el_a->priority > el_b->priority;
}