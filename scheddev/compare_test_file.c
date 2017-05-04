#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "include/lwt.h"

#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))



void *
lm_fn_null(void *d)
{ return NULL; }


void
test_lwt_yield(void)
{
	int i;
	unsigned long long start, end;

	for (i = 0 ; i < 1000 ; i++){
		lwt_create(lm_fn_null, NULL, LWT_NOJOIN);
	}

	rdtscll(start);
	lwt_yield(LWT_NULL);
	rdtscll(end);


	printc("[PERF] %5lld <- yield 1000 in lwt\n", (end-start));


}


void *
lm_fn_chan(lwt_chan_t to)
{
	lwt_chan_t from;
	int i;
	from = (lwt_chan_t)lwt_chan(0);
	
	lwt_snd_chan(to, from);


	for (i = 0 ; i < 10000 ; i++) {
		lwt_snd(to, (void*)1);
		lwt_rcv(from);
	}
	lwt_chan_deref(from);
	
	return NULL;
}

void
test_lwt_channel()
{
	lwt_chan_t from, to;
	lwt_t t;
	int i;
	unsigned long long start, end;


	from = (lwt_chan_t)lwt_chan(0);

	t    = lwt_create_chan((lwt_chan_fn_t)lm_fn_chan, from, LWT_JOIN); 

	to   = (lwt_chan_t)lwt_rcv_chan(from);
	
	
	rdtscll(start);
	for (i = 0 ; i < 10000 ; i++) {
		lwt_rcv(from);
		lwt_snd(to, (void*)2);
	}
	lwt_chan_deref(to);
	rdtscll(end);
	printc("[PERF] %5lld <- channel in lwt\n",
	       (end-start)/(10000*2));
	lwt_join(t);
}

void *
lm_fn_identity(void *d)
{ return d; }

void
test_lwt_create(void)
{
	int i;
	unsigned long long start, end;

	rdtscll(start);
	for (i = 0 ; i < 10000 ; i++){
		lwt_create(lm_fn_identity, NULL, LWT_NOJOIN);
	}
	rdtscll(end);


	printc("[PERF] %5lld <- thd_create in lwt\n", (end-start));
}


void*
lm_fn_multiwait(void *d)
{
	assert(d);

	int i;

	for (i = 0 ; i < 10000 ; i++) {
		if ((i % 7) == 0) {
			int j;

			for (j = 0 ; j < (i % 8) ; j++) lwt_yield(LWT_NULL);
		}
		lwt_snd((lwt_chan_t)d, (void*)lwt_id(lwt_current()));
	}
}



void
test_lwt_cgrp()
{
	lwt_chan_t cs[100];
	lwt_t ts[100];
	int i;
	lwt_cgrp_t g;
	unsigned long long start, end;

	g = lwt_cgrp();
	assert(g);
	
	for(i = 0 ; i < 100 ; i++) {
		cs[i] = lwt_chan(0);
		assert(cs[i]);
		
		ts[i] = lwt_create_chan(lm_fn_multiwait, cs[i],LWT_NOJOIN);
		assert(ts[i]);

		lwt_chan_mark_set(cs[i], (void*)lwt_id(ts[i]));
		assert(0 == lwt_cgrp_add(g, cs[i]));
	}

	assert(lwt_cgrp_free(g) == -1);

	rdtscll(start);
	for(i = 0 ; i < 10000 * 100; i++) {
		lwt_chan_t c;
		int r;
		c = lwt_cgrp_wait(g);
		//assert(c);
		
		r = (int)lwt_rcv(c);
		//assert(r == (int)lwt_chan_mark_get(c));
	}
	rdtscll(end);
	printc("[PERF] %5lld <- grp in lwt, size %d\n", (end-start)/(10000*100),100);
	
	for(i = 0 ; i < 100 ; i++) {
	 	lwt_cgrp_rem(g, cs[i]);
	 	lwt_chan_deref(cs[i]);
	}

	assert(0 == lwt_cgrp_free(g));
	
	
	return;
}


void
run_lm_tests()
{
//	test_lwt_yield();
	test_lwt_channel();
//	test_lwt_create();
	test_lwt_cgrp();
}
