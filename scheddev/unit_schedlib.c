/*
 * Copyright 2016, Phani Gadepalli and Gabriel Parmer, GWU, gparmer@gwu.edu.
 *
 * This uses a two clause BSD License.
 */

#include <stdio.h>
#include <string.h>
#include <cos_component.h>
#include <cobj_format.h>
#include <cos_defkernel_api.h>
#include "include/lwt.h"
#include <sl.h>

#undef assert
#define assert(node) do { if (unlikely(!(node))) { debug_print("assert error in @ "); *((int *)0) = 0; } } while (0)
#define PRINT_FN prints
#define debug_print(str) (PRINT_FN(str __FILE__ ":" STR(__LINE__) ".\n"))
#define BUG() do { debug_print("BUG @ "); *((int *)0) = 0; } while (0);
#define SPIN(iters) do { if (iters > 0) { for (; iters > 0 ; iters -- ) ; } else { while (1) ; } } while (0)

static void
cos_llprint(char *s, int len)
{ call_cap(PRINT_CAP_TEMP, (int)s, len, 0, 0); }

int
prints(char *s)
{
	int len = strlen(s);

	cos_llprint(s, len);

	return len;
}

int __attribute__((format(printf,1,2)))
printc(char *fmt, ...)
{
	  char s[128];
	  va_list arg_ptr;
	  int ret, len = 128;

	  va_start(arg_ptr, fmt);
	  ret = vsnprintf(s, len, fmt, arg_ptr);
	  va_end(arg_ptr);
	  cos_llprint(s, ret);

	  return ret;
}

#define N_TESTTHDS 8
#define WORKITERS  10000

void
test_thd_fn(void *data)
{
	while (1) {
		int workiters = WORKITERS * ((int)data);

		printc("kthd id %d %d %d\n",cos_thdid(), (int)data,workiters);
		SPIN(workiters);
		sl_thd_yield(0);
	}
}

void
test_yields(void)
{
	int                     i;
	struct sl_thd          *threads[N_TESTTHDS];
	union sched_param       sp    = {.c = {.type = SCHEDP_PRIO, .value = 10}};

	for (i = 0 ; i < /*N_TESTTHDS*/2 ; i++) {
		threads[i] = sl_thd_alloc(test_thd_fn, (void *)(i + 1));
		assert(threads[i]);
		sl_thd_param_set(threads[i], sp.v);
	}
}

void
test_high(void *data)
{
	struct sl_thd *t = data;

	while (1) {
		sl_thd_yield(t->thdid);
		printc("h");
	}
}

void
test_low(void *data)
{
	while (1) {
		int workiters = WORKITERS * 10;
		SPIN(workiters);
		printc("l");
	}
}

void
test_blocking_directed_yield(void)
{
	struct sl_thd          *low, *high;
	union sched_param       sph = {.c = {.type = SCHEDP_PRIO, .value = 5}};
	union sched_param       spl = {.c = {.type = SCHEDP_PRIO, .value = 10}};

	low  = sl_thd_alloc(test_low, NULL);
	high = sl_thd_alloc(test_high, low);
	sl_thd_param_set(low, spl.v);
	sl_thd_param_set(high, sph.v);
}

void 
fn(void)
{
	int i;
	for(i = 0; i < 100; i++)
	{
		//printc("lwt curr id %d\n",lwt_current()->id);
		printc("%d ",i+1);
		lwt_yield(NULL);
	}
}

void 
fn_cpy(void)
{
	int i;
	for(i = 0; i < 100; i++)
	{
		//printc("lwt curr id %d\n",lwt_current()->id);
		printc("%d ",100 - i);
		lwt_yield(NULL);
	}
}

void test_remote_yield()
{
	lwt_chan_t c1 = lwt_chan(0);
	lwt_chan_t c2 = lwt_chan(0);
//	lwt_chan_t c3 = lwt_chan(0);
	assert(lwt_kthd_create(fn,c1) == 0);
	assert(lwt_kthd_create(fn_cpy,c2) == 0);
//	assert(lwt_kthd_create(fn,c3) == 0);
}

void 
fn2(lwt_chan_t to)
{
	lwt_chan_t from;
	int i;
	
	from = lwt_chan(0);
	lwt_snd_chan(to, from);
	assert(from->snd_cnt);
	for (i = 0 ; i < 10000 ; i++) {
		lwt_snd(to, (void*)1);
		assert(2 == (int)lwt_rcv(from));
	}
	lwt_chan_deref(from);
}

void
fn1(lwt_chan_t from)
{
	lwt_chan_t to;
	int i;
	unsigned long long start, end;

	to   = lwt_rcv_chan(from);
	assert(to->snd_cnt);
	rdtscll(start);
	for (i = 0 ; i < 10000 ; i++) {
		assert(1 == (int)lwt_rcv(from));
		lwt_snd(to, (void*)2);
	}
	lwt_chan_deref(to);
	rdtscll(end);
	printc("[PERF] %5lld <- snd+rcv (buffer size %d)\n", 
	       (end-start)/(10000*2), from->data_buffer.size - 1);
	lwt_join(to->rcv_thd);
}

void 
test_remote_communication(int sz)
{
	lwt_chan_t c1 = lwt_chan(sz);	
	assert(lwt_kthd_create(fn1,c1) == 0);
	assert(lwt_kthd_create(fn2,c1) == 0);
}

void
fn_print(void* data)
{
	printc("[PRINT] target kid %d self kid %d lwt id%d\n",data,lwt_head->tid,lwt_head->id);
	assert(data == lwt_head->tid);
}
void 
fn_rcv_thd(lwt_chan_t from)
{
	int i = 0;
	while(i < 10)
	{
		lwt_t lwt = lwt_rcv_thd(from);
		assert(lwt);
		i++;
	}
}

void
fn_snd_thd(lwt_chan_t to)
{
	int i = 0;
	while(i < 10)
	{
		lwt_t lwt = lwt_create((lwt_fn_t)fn_print,(void*)to->rcv_thd->tid,LWT_NOJOIN); 
		lwt_snd_thd(to,lwt);
		i++;
	}
	lwt_t lwt = lwt_create((lwt_fn_t)fn_print,(void*)lwt_head->tid,LWT_NOJOIN); 
}
void 
test_thd_snd()
{
	lwt_chan_t c1 = lwt_chan(0);	
	assert(lwt_kthd_create(fn_rcv_thd,c1) == 0);
	assert(lwt_kthd_create(fn_snd_thd,c1) == 0);
}

void *
fn_asnd(lwt_chan_t to)
{
	int i;
	
	for (i = 0 ; i < 100 ; i++) lwt_snd(to, (void*)(i+1));
	lwt_chan_deref(to);
	
	return NULL;
}

void
fn_arcv(lwt_chan_t from)
{
	int i;
	unsigned long long start, end;

	assert(LWT_ACTIVE == lwt_current()->status);
	assert(from);
	rdtscll(start);
	for (i = 0 ; i < 100 ; i++) assert(i+1 == (int)lwt_rcv(from));
	rdtscll(end);
	printc("[PERF] %5lld <- asynchronous snd->rcv (buffer size %d)\n",
	       (end-start)/(100*2), from->data_buffer.size - 1);
	lwt_chan_deref(from);
}

void 
test_remote_asy(int chsz)
{
	lwt_chan_t c = lwt_chan(chsz);
	assert(lwt_kthd_create(fn_arcv,c) == 0);
	assert(lwt_kthd_create(fn_asnd,c) == 0);
}

void*
fn_grp_snd(void *d)
{
	assert(d);
	int i;

	for (i = 0 ; i < 10000 ; i++) 
	{
		if ((i % 7) == 0) 
		{
			int j;
			for (j = 0 ; j < (i % 8) ; j++) 
				lwt_yield(LWT_NULL);
		}
		lwt_snd((lwt_chan_t)d, ((lwt_chan_t)d)->id);
	}

}

void*
fn_multisend(lwt_chan_t c)
{
	int i;
	lwt_chan_t ct;

	for(i = 0; i < 30; i++)
	{
		ct = lwt_rcv_chan(c);
		lwt_create_chan((lwt_chan_fn_t)fn_grp_snd,ct,LWT_NOJOIN);
	}

	lwt_yield(LWT_NULL);

	return NULL;
}

void*
fn_multiwait(lwt_chan_t c)
{
	int i;
			
	lwt_chan_t *cs = (lwt_chan_t*)kalloc(sizeof(lwt_chan_t)*10);
				
	lwt_cgrp_t g;
	unsigned long long start, end;
	g = lwt_cgrp();
	assert(g);

	for(i = 0; i < 10; i++)
	{
		cs[i] = lwt_chan(0);
		lwt_chan_mark_set(cs[i], (cs[i])->id);
		lwt_snd_chan(c,cs[i]);
		assert(0 == lwt_cgrp_add(g, cs[i]));
	}

	assert(lwt_cgrp_free(g) == -1);

	rdtscll(start);
	for(i = 0 ; i < 10000 * 10; i++) 
	{
		lwt_chan_t c;
		int r;
		c = lwt_cgrp_wait(g);
		assert(c);
		r = (int)lwt_rcv(c);
		assert(r == (int)lwt_chan_mark_get(c));
	}
	rdtscll(end);
	printc("[PERF] %5lld <- multiwait (group size %d)\n", (end-start)/(10000*10), 100);

	for(i = 0 ; i < 10; i++) 
	{
		lwt_cgrp_rem(g, cs[i]);
		lwt_chan_deref(cs[i]);
	}
	assert(0 == lwt_cgrp_free(g));
	return NULL;

}

void
test_kthd_multiwait()
{
	lwt_chan_t c = lwt_chan(0);
			
	assert(0 == lwt_kthd_create((lwt_fn_t)fn_multisend, c));
	assert(0 == lwt_kthd_create((lwt_fn_t)fn_multiwait, c));

}

void* 
fn_record(void)
{
	unsigned long long start, end;
	rdtscll(start);
	lwt_yield(NULL);
	rdtscll(end);
	printc("[PERF] %5lld <- thd_yield in lwt\n", (end-start)/11);
}

void* 
fn_nulll(void)
{lwt_yield(NULL);}

void
test_kthd_create(void)
{
	int i;
	unsigned long long start, end;

	lwt_chan_t c = lwt_chan(0);
	rdtscll(start);
	for (i = 0 ; i < 10 ; i++){
		lwt_kthd_create(fn_nulll, c, LWT_NOJOIN);
	}
	rdtscll(end);


	printc("[PERF] %5lld <- thd_create in lwt\n", (end-start)/10);
}

void
test_kthd_yield(void)
{
	int i;
	
	lwt_chan_t c = lwt_chan(0);
	lwt_kthd_create(fn_record, c, LWT_NOJOIN);
	for (i = 0 ; i < 10 ; i++){
		lwt_kthd_create(fn_nulll, c, LWT_NOJOIN);
	}
}

void
cos_init(void)
{
	struct cos_defcompinfo *defci = cos_defcompinfo_curr_get();
	struct cos_compinfo    *ci    = cos_compinfo_get(defci);

	printc("Unit-test for the scheduling library (sl)\n");
	cos_meminfo_init(&(ci->mi), BOOT_MEM_KM_BASE, COS_MEM_KERN_PA_SZ, BOOT_CAPTBL_SELF_UNTYPED_PT);
	cos_defcompinfo_init();
	sl_init();

	kinit();
//	test_yields();
//	test_blocking_directed_yield();
//	maintest();

//	test_remote_yield();

//	test_remote_communication(0);
//	test_remote_communication(1);
//	test_remote_communication(4);
//	test_remote_communication(16);
//	test_remote_communication(64);
//	test_thd_snd();
//	test_remote_asy(10);
	test_kthd_multiwait();
//	test_kthd_yield();
//	test_kthd_create();
	sl_sched_loop();
	printc("testfinished\n");

	assert(0);

	return;
}
