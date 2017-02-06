#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "lwt.h"

#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))

#define ITER 10000 

/* 
 * My output on an Intel Core i5-2520M CPU @ 2.50GHz:
 *
 * [PERF] 120 <- fork/join
 * [PERF] 13 <- yield
 * [TEST] thread creation/join/scheduling
 */

void *
fn_bounce(void *d) 
{
	int i;
	unsigned long long start, end;
#ifdef __DEBUG
	printf("current id = %d, ip = %d, sp = %d\n",lwt_id(lwt_current()),lwt_current()->ip, lwt_current()->sp);
	DEBUG();
	printf("prev id = %d, ip = %d, sp = %d\n",lwt_current()->prev->id,lwt_current()->prev->ip,lwt_current()->prev->sp);
#endif
	lwt_yield(LWT_NULL);
#ifdef __DEBUG

	printf("current id = %d, ip = %d, sp = %d\n",lwt_id(lwt_current()),lwt_current()->ip, lwt_current()->sp);
	DEBUG();
	printf("prev id = %d, ip = %d, sp = %d\n",lwt_current()->prev->id,lwt_current()->prev->ip,lwt_current()->prev->sp);
#endif
	lwt_yield(LWT_NULL);
#ifdef __DEBUG

	printf("current id = %d, ip = %d, sp = %d\n",lwt_id(lwt_current()),lwt_current()->ip, lwt_current()->sp);
	DEBUG();
	printf("prev id = %d, ip = %d, sp = %d\n",lwt_current()->prev->id,lwt_current()->prev->ip,lwt_current()->prev->sp);
#endif
	
	rdtscll(start);
	for (i = 0 ; i < ITER ; i++){ lwt_yield(LWT_NULL);/*printf("round %d\n",i);*/}
	rdtscll(end);
#ifdef __DEBUG
	printf("prev id = %d, ip = %d, sp = %d\n",lwt_current()->prev->id,lwt_current()->prev->ip,lwt_current()->prev->sp);
	printf("current id = %d, ip = %d, sp = %d\n",lwt_id(lwt_current()),lwt_current()->ip, lwt_current()->sp);
	DEBUG();

#endif
	lwt_yield(LWT_NULL);
#ifdef __DEBUG
	printf("prev id = %d, ip = %d, sp = %d\n",lwt_current()->prev->id,lwt_current()->prev->ip,lwt_current()->prev->sp);
	printf("current id = %d, ip = %d, sp = %d\n",lwt_id(lwt_current()),lwt_current()->ip, lwt_current()->sp);
	DEBUG();
	
#endif
	lwt_yield(LWT_NULL);


	if (!d) printf("[PERF] %5lld <- yield\n", (end-start)/(ITER*2));


	return NULL;
}

void *
fn_null(void *d)
{ return NULL; }

#define IS_RESET()						\
        assert( lwt_info(LWT_INFO_NTHD_RUNNABLE) == 1 &&	\
		lwt_info(LWT_INFO_NTHD_ZOMBIES) == 0 &&		\
		lwt_info(LWT_INFO_NTHD_BLOCKED) == 0)

void
test_perf(void)
{
	lwt_t chld1, chld2;
	int i;
	unsigned long long start, end;


	/* Performance tests */
	rdtscll(start);
	for (i = 0 ; i < ITER ; i++) {
		chld1 = lwt_create(fn_null, NULL);	
		lwt_join(chld1);
	}
	rdtscll(end);
	printf("[PERF] %5lld <- fork/join\n", (end-start)/ITER);
#ifdef __DEBUG
	DEBUG();
#endif
	printf("run = %d,blocked = %d,died = %d\n", lwt_info(LWT_INFO_NTHD_RUNNABLE),lwt_info(LWT_INFO_NTHD_BLOCKED),lwt_info(LWT_INFO_NTHD_ZOMBIES));
	IS_RESET();

	chld1 = lwt_create(fn_bounce, (void*)1);
	chld2 = lwt_create(fn_bounce, NULL);
//	lwt_yield(LWT_NULL);
	lwt_join(chld1);
	lwt_join(chld2);

	printf("run = %d,blocked = %d,died = %d\n", lwt_info(LWT_INFO_NTHD_RUNNABLE),lwt_info(LWT_INFO_NTHD_BLOCKED),lwt_info(LWT_INFO_NTHD_ZOMBIES));
	IS_RESET();
}

void *
fn_identity(void *d)
{ return d; }

void *
fn_nested_joins(void *d)
{
	lwt_t chld;

	if (d) {
		lwt_yield(LWT_NULL);
		lwt_yield(LWT_NULL);
		assert(lwt_info(LWT_INFO_NTHD_RUNNABLE) == 1);
		lwt_die(NULL);
	}
	chld = lwt_create(fn_nested_joins, (void*)1);
	lwt_join(chld);
}

volatile int sched[2] = {0, 0};
volatile int curr = 0;

void *
fn_sequence(void *d)
{
	int i, other, val = (int)d;
#ifdef __DEBUG
	DEBUG();
#endif
	for (i = 0 ; i < ITER ; i++) {
		other = curr;
#ifdef __DEBUG
	DEBUG();
#endif
		curr  = (curr + 1) % 2;
#ifdef __DEBUG
	DEBUG();
#endif
		sched[curr] = val;
#ifdef __DEBUG
	DEBUG();
	printf("left = %d,val = %d\n",sched[other],val);
#endif
		assert(sched[other] != val);
#ifdef __DEBUG
	DEBUG();
#endif
		lwt_yield(LWT_NULL);
#ifdef __DEBUG
	DEBUG();
#endif
	}


	return NULL;
}

void *
fn_join(void *d)
{
	lwt_t t = (lwt_t)d;
	void *r;

	r = lwt_join(d);
	assert(r == (void*)0x37337);
}

void
test_crt_join_sched(void)
{
	lwt_t chld1, chld2;

	printf("[TEST] thread creation/join/scheduling\n");

	/* functional tests: scheduling */
	lwt_yield(LWT_NULL);

#ifdef __DEBUG
	DEBUG();
#endif
	chld1 = lwt_create(fn_sequence, (void*)1);
	chld2 = lwt_create(fn_sequence, (void*)2);
	lwt_join(chld2);
	lwt_join(chld1);	
	IS_RESET();

#ifdef __DEBUG
	DEBUG();
#endif
	/* functional tests: join */
	chld1 = lwt_create(fn_null, NULL);
	lwt_join(chld1);
	IS_RESET();

#ifdef __DEBUG
	DEBUG();
#endif
	chld1 = lwt_create(fn_null, NULL);

#ifdef __DEBUG
	DEBUG();
	printf("chld1 id = %d, status = %d\n",chld1->id,chld1->status);
#endif
	lwt_yield(LWT_NULL);
#ifdef __DEBUG
	DEBUG();
#endif
	lwt_join(chld1);
	IS_RESET();

#ifdef __DEBUG
	DEBUG();
#endif
	chld1 = lwt_create(fn_nested_joins, NULL);
	lwt_join(chld1);
	IS_RESET();

#ifdef __DEBUG
	DEBUG();
#endif
	/* functional tests: join only from parents */
	chld1 = lwt_create(fn_identity, (void*)0x37337);
	chld2 = lwt_create(fn_join, chld1);
	lwt_yield(LWT_NULL);
	lwt_yield(LWT_NULL);
	lwt_join(chld2);
	//lwt_join(chld1);
	IS_RESET();

#ifdef __DEBUG
	DEBUG();
#endif
	/* functional tests: passing data between threads */
	chld1 = lwt_create(fn_identity, (void*)0x37337);
	assert((void*)0x37337 == lwt_join(chld1));
	IS_RESET();

	/* functional tests: directed yield */
	chld1 = lwt_create(fn_null, NULL);
	lwt_yield(chld1);
	assert(lwt_info(LWT_INFO_NTHD_ZOMBIES) == 1);
	lwt_join(chld1);
	IS_RESET();
}

int
main(void)
{
//	assert(0 == 1);
	test_perf();
	test_crt_join_sched();

	return 0;
}
