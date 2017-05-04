#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<sl.h>
#include"include/kalloc.h"
#include"include/lwt.h"

void __lwt_start();
void __lwt_schedule(void);
static inline void __lwt_dispatch(lwt_t curr, lwt_t next);	
void __init_counter(void);
void __init_thread_head(void);


void 
lwt_init()  
{  
	kinit();
	__init_thread_head();


}  	


void
__init_thread_head()
{
	uint size = sizeof(struct _lwt_t) + STACK_SIZE;
	lwt_head = (lwt_t) kalloc(size);

	assert(lwt_head);
	ps_list_init_d(lwt_head);
	lwt_head->ip = (ulong)0;
	lwt_head->sp = (ulong)0;
	lwt_head->id = gcounter.lwt_count++;
	lwt_head->status = LWT_ACTIVE;
	lwt_head->tid = cos_thdid();
	gcounter.runable_counter++;

}

lwt_t 
lwt_create(lwt_fn_t fn, void *data, lwt_flags_t flags)
{

	lwt_t lwt_new;

	assert(fn);
	
	//malloc the new thread and stack
	uint size = sizeof(struct _lwt_t) + STACK_SIZE;
	lwt_new = (lwt_t) kalloc(size);
	assert(lwt_new);
	lwt_new->id = gcounter.lwt_count++;
	lwt_new->ip = (ulong) (&__lwt_start);
	lwt_new->sp = (ulong)lwt_new + size - 4; 		
	lwt_new->status = LWT_ACTIVE;
	lwt_new->lwt_nojoin = flags;
	lwt_new->fn = fn;
	lwt_new->data = data;
	//lwt_new->kthd = sl_thd_curr();
	lwt_new->tid = lwt_head->tid;
	lwt_new->joiner = NULL;
	lwt_new->target = NULL;
	lwt_new->return_val = NULL;

	gcounter.runable_counter++;

	
	ps_list_add_d(ps_list_prev_d(lwt_head),lwt_new);

	return lwt_new;
}

void*
lwt_join(lwt_t lwt)
{	
	void *temp_data;

	assert(lwt);
	if(lwt->lwt_nojoin)
		return NULL;
	lwt_head->joiner = lwt;
	lwt->target = lwt_head;
	if(lwt->status == LWT_ACTIVE)
	{
		lwt_head->status = LWT_BLOCKED;
		gcounter.blocked_counter++;
		gcounter.runable_counter--;
		lwt_yield(NULL);
		/*lwt_chan_t c = lwt_chan(0);
		c->snd_cnt = 1;
		lwt_head->data = c;
		lwt_rcv(c);
		lwt_chan_deref(c);*/
	}
	temp_data = lwt->return_val;
	ps_list_rem_d(lwt);
	kfree((void *)lwt,sizeof(struct _lwt_t) + STACK_SIZE);
	gcounter.died_counter--;

	return temp_data;

}

void
lwt_die(void *data)
{	

	/*
	 * if the current thread is the joiner of a specific thread,
	 * set this blocked thread active
	 */

	if(lwt_head->target != LWT_NULL)// && lwt_head->target->status == LWT_BLOCKED)
	{
		gcounter.blocked_counter--;
		lwt_head->target->status = LWT_ACTIVE;
		gcounter.runable_counter++;
		/*lwt_snd((lwt_chan_t)lwt_head->target->data,(void *)1);
		lwt_chan_deref((lwt_chan_t)lwt_head->target->data);*/
	}
		
	lwt_head->return_val = (void *)data;
	gcounter.runable_counter--;

	if(lwt_head->lwt_nojoin)
	{
		lwt_t tmp_curr = lwt_head;
		do{
			lwt_head = ps_list_next_d(lwt_head);
		}while(lwt_head->status != LWT_ACTIVE);
		
		assert(tmp_curr != lwt_head);	
		ps_list_rem_d(tmp_curr);
		kfree((void *)tmp_curr,sizeof(struct _lwt_t) + STACK_SIZE);
		struct _lwt_t trash;
		__lwt_dispatch(&trash,lwt_head);
	}
	else
	{
		lwt_head->status = LWT_DEAD;
		gcounter.died_counter++;
		__lwt_schedule();	
	}
}

int
lwt_yield(lwt_t lwt)
{
	if(lwt == NULL)
	{
		__lwt_schedule();
	
	}
	else if (lwt->status == LWT_ACTIVE)
	{
		if(lwt->tid == lwt_head->tid)
		{
			lwt_t tmp_curr = lwt_head;
			lwt_head = lwt;
			__lwt_dispatch(tmp_curr,lwt_head);
		}_
		else
		{
			__lwt_schedule();
		}
	}
	return -1;
}

lwt_t
lwt_current(void)
{
	return lwt_head;
}

/*
 * gets the thread id of a specified thread.
 * returns -1 if the thread not exists
 */
int 
lwt_id(lwt_t lwt)
{
	assert(lwt);

	if (!lwt)
	{
		return -1;
	}
	return lwt->id;
}

/*
 * get the number of threads that are either runnable,
 * blocked (i.e. that are joining, on a lwt that hasn't died),
 *  or that have died
 */
int
lwt_info(lwt_info_t t)
{
	switch (t) {
		case LWT_INFO_NTHD_RUNNABLE:
			return gcounter.runable_counter;
		case LWT_INFO_NTHD_BLOCKED:
			return gcounter.blocked_counter;
		case LWT_INFO_NTHD_ZOMBIES:
			return gcounter.died_counter;							
		case LWT_INFO_NCHAN:
			return gcounter.nchan_counter;							
		case LWT_INFO_NSNDING:
			return gcounter.nsnding_counter;							
		case LWT_INFO_NRCVING:
			return gcounter.nrcving_counter;
		default:
			return -1;			

	}
}


void 
__lwt_start()
{
	void *return_val = lwt_head->fn(lwt_head->data);
	lwt_die(return_val);
}

void
__lwt_schedule(void)
{
	lwt_t temp;
	temp = lwt_head;

	do{
		lwt_head = ps_list_next_d(lwt_head);
	}while(lwt_head->status != LWT_ACTIVE);

	if (temp != lwt_head)
	{
		__lwt_dispatch(temp, lwt_head);

	}
	return;	
}

static inline void 
__lwt_dispatch(lwt_t curr, lwt_t next)	
{
	__asm__ __volatile__(	
			"pushal\n\t" 						//PUSH ALL other register
			"movl $1f, (%%eax)\n\t" 			//save IP to TCB
			"movl %%esp, (%%ebx)\n\t"			//save SP to TCB
			"movl %%edx, %%esp\n\t"				//recover the SP
			"jmp %%ecx\n\t"						//recover the IP
			"1:"							//LABEL
			"popal"
			:: "a"(&curr->ip), "b"(&curr->sp), "c"(next->ip), "d"(next->sp)
			: "cc", "memory");
			//for switching back to old thread, nested?
	return;	
}

/* 
 * Currently assume that sz is always 0. 
 * This function uses malloc to allocate 
 * the memory for the channel. 
 */

lwt_chan_t 
lwt_chan(int sz)
{
	lwt_chan_t new;
	assert(sz >= 0);
	uint size = sizeof(struct lwt_channel) + (sz + 1) * sizeof(void *);
	new = (lwt_chan_t) kalloc(size);
	assert(new);
	new->data_buffer.size = sz + 1;
	new->data_buffer.start = 0;
	new->data_buffer.end = 0;
	new->data_buffer.data = (void*) new + sizeof(struct lwt_channel);
	new->snd_cnt = 0;
	new->snd_thds = NULL;
	new->id = gcounter.nchan_id++;
	gcounter.nchan_counter++;
	new->rcv_thd = lwt_head;
	new->iscgrp = 0;
	new->cgrp = NULL;
	return new;
}

/*
 * Deallocate the channel if no threads still 
 * have references to the channel.
 */

void 
lwt_chan_deref(lwt_chan_t c)
{
	assert(c);
	if(c->snd_cnt > 0)
	{
		c->snd_cnt--;
	}
	else if (c->rcv_thd == lwt_head && c->rcv_thd->status == LWT_ACTIVE) 
	{
		gcounter.nchan_counter--;
		c->rcv_thd = NULL; 	//will also set "rcv_thd" as NULL
		kfree((void*)c,sizeof(struct lwt_channel) + c->data_buffer.size * sizeof(void *));
	}
}

int 
lwt_snd(lwt_chan_t c, void *data)
{
	//data being NULL or rcv being null is illegal
	assert(data && c);
/*
	if (c->rcv_thd == NULL)
   	{
		return -1;
	}*/

	gcounter.nsnding_counter++;
	gcounter.runable_counter--;
	if(c->data_buffer.size == 1)
	{

		//block current thread
		while (c->rcv_thd->status == LWT_ACTIVE)//blocked == 0) 
		{
			lwt_yield(LWT_NULL);								
		}
		//if there is node that is new added without filling the data,fill it 
		clist_t new_clist;
		uint size = sizeof(struct clist_head);
		new_clist = (clist_t) kalloc(size);
		assert(new_clist);	
		new_clist->thd = lwt_head;
		new_clist->data = data;					

		if(c->snd_thds == NULL)
		{
			ps_list_init_d(new_clist);
			c->snd_thds = new_clist;																	
		}
		else
		{
			ps_list_add_d(ps_list_prev_d(c->snd_thds),new_clist);															
		}
	}
	else
	{
		while((c->data_buffer.end + 1)%c->data_buffer.size == c->data_buffer.start) 
		{
			lwt_yield(LWT_NULL);							
		}	
		c->data_buffer.data[c->data_buffer.end] = data;
		c->data_buffer.end = (c->data_buffer.end + 1)%c->data_buffer.size;

	}
	//add the event
	if (c->iscgrp && (c->data_buffer.start + 1)%c->data_buffer.size == c->data_buffer.end)
	{
		if(c->cgrp->events == NULL)
		{
			ps_list_init_d(c);
			c->cgrp->events = c;
		}
		else
		{
			ps_list_add_d(ps_list_prev_d(c->cgrp->events),c);
		}
	}

	c->rcv_thd->status = LWT_ACTIVE;
	struct sl_thd* t = sl_thd_lkup(c->rcv_thd->tid);

	if(t->state == SL_THD_BLOCKED)
	{
		t->state = SL_THD_RUNNABLE;
		sl_mod_wakeup(sl_mod_thd_policy_get(t));
	}

	gcounter.nsnding_counter--;
	gcounter.runable_counter++;
	return 0;
}

void 
*lwt_rcv(lwt_chan_t c)
{
	assert(c != NULL);
	void* temp;
	c->rcv_thd = lwt_head;
/*	if (c->rcv_thd != lwt_head) 
	{
		return NULL;
	}*/

	c->rcv_thd->status = LWT_WAITING;//blocked = 1;
	gcounter.nrcving_counter++;
	gcounter.runable_counter--;
	if(c->data_buffer.size == 1)
	{
		//if snd_thds is null, block the reciever to wait for sender
		while (c->snd_thds == NULL)//ps_list_head_empty(&c->snd_head)) 
		{
			lwt_yield(LWT_NULL);						
		}
		temp = c->snd_thds->data;
		if(ps_list_singleton_d(c->snd_thds))
		{
			assert(c->snd_thds);
			kfree((void *)c->snd_thds,sizeof(struct clist_head));
			c->snd_thds = NULL;														
		}
		else
		{
			clist_t tmp = c->snd_thds;
			c->snd_thds = ps_list_next_d(tmp);
			ps_list_rem_d(tmp);
			assert(c->snd_thds);
			kfree((void *)c->snd_thds,sizeof(struct clist_head));
		}
	}
	else
	{
		//if empty, block rcv, remove c from ready queue
		while(c->data_buffer.start == c->data_buffer.end) 
		{
			lwt_yield(LWT_NULL);
		}
		temp = c->data_buffer.data[c->data_buffer.start];
		c->data_buffer.start = (c->data_buffer.start + 1)%c->data_buffer.size;
	}
	if (c->iscgrp && c->data_buffer.start == c->data_buffer.end)
	{
		if(c->cgrp->events != NULL) 
		{
			if(ps_list_singleton_d(c))
			{
				c->cgrp->events = NULL;
			}
			else
			{
				c->cgrp->events = ps_list_next_d(c);
				ps_list_rem_d(c);
			}
		}			
	} 
	gcounter.nrcving_counter--;
	gcounter.runable_counter++;
	c->rcv_thd->status = LWT_ACTIVE;
	//remove the event
	return temp;
}

/* 
 * a channel is sent over the channel (sending is sent over c). 
 * This is used for reference counting to determine 
 * when to deallocate the channel. 
 */

int 
lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending)
{
	//data being NULL or rcv being null is illegal
	assert(c && sending);
	sending->snd_cnt++;
	sending->rcv_thd = lwt_head;

	return lwt_snd(c,sending);
}

/* Same as for lwt rcv except a channel is sent over 
the channel.*/

lwt_chan_t 
lwt_rcv_chan(lwt_chan_t c)
{
	return lwt_rcv(c);
}

/*
 * This function, like lwt create, creates a new thread. 
 * The difference is that instead of passing a void * to that new thread, 
 * it passes a channel.
 */

lwt_t 
lwt_create_chan(lwt_chan_fn_t fn, lwt_chan_t c, lwt_flags_t flag)
{
	assert(fn);	
	assert(c);
	
	lwt_t lwt = lwt_create((lwt_fn_t)fn,(void*)c,flag);
	
	c->snd_cnt++;

	return lwt;
}

lwt_cgrp_t 
lwt_cgrp(void)
{
	lwt_cgrp_t new;
    uint size = sizeof(struct cgroup);
	new = (lwt_cgrp_t) kalloc(size);
	assert(new);
	new->n_chan = 0;
	new->events = NULL;
	return new;
}

int 
lwt_cgrp_free(lwt_cgrp_t cgrp)
{
	assert(cgrp);
	if (NULL == cgrp->events && 0 == cgrp->n_chan)
	{
		kfree((void *)cgrp,sizeof(struct cgroup));
		return 0;							    
	}
    return -1;

}

int lwt_cgrp_add(lwt_cgrp_t cgrp, lwt_chan_t c)
{
	if (c->iscgrp == 1) return -1; //A channel can be added into only one group 
	cgrp->n_chan++;
	c->iscgrp = 1;
	c->cgrp = cgrp;		 
	return 0;
}

int lwt_cgrp_rem(lwt_cgrp_t cgrp, lwt_chan_t c)
{
	assert(!c->snd_thds);
	if (c->snd_thds != NULL || c->data_buffer.start != c->data_buffer.end) 
		return 1; //cgrp has a pending event
	if(cgrp != c->cgrp)
		return -1;	
	cgrp->n_chan--;
	c->iscgrp = 0;
	c->cgrp = NULL;
	return 0;
}

lwt_chan_t lwt_cgrp_wait(lwt_cgrp_t cgrp)
{
	lwt_head->status = LWT_WAITING;
	while (cgrp->events == NULL) 
	{
		lwt_yield(NULL);			    
	}
	lwt_head->status = LWT_ACTIVE;
	return cgrp->events;

}

void lwt_chan_mark_set(lwt_chan_t c, void *data)
{
	c->mark = data;
	return;

}

void *lwt_chan_mark_get(lwt_chan_t c)
{
	return c->mark;

}

void *__lwt_kthd_entry(void* data)
{
	__init_thread_head();
	kthd_parm_t parm = (kthd_parm_t) data;
	parm->lwt = lwt_head;
	//global_counter_t* tmpcounter = gcounter;
	lwt_create(parm->fn,parm->c, LWT_NOJOIN);

	int id = cos_thdid();
	while(1)
	{
		sl_cs_enter();

		lwt_head = parm->lwt;
	//	gcounter = tmpcounter;
		lwt_yield(NULL);
		
		sl_cs_exit();
		
		lwt_t tmp = parm->lwt;
		int anum = 0;
		do{
			if(tmp->status == LWT_ACTIVE)
				anum++;
			tmp = ps_list_next_d(tmp);
		}while(tmp != parm->lwt);
	//	int ifnext = ps_list_next_d(parm->lwt) != parm->lwt;
	//	if(!ifnext)
		if(anum == 1)
//		assert(gcounter.runable);
//		if(gcounter.runable == 1)
		{
			sl_thd_block(0);
		}
		//sl_cs_exit_schedule_nospin();

		sl_thd_yield(0);
	}
}

int lwt_kthd_create(lwt_fn_t fn, lwt_chan_t c)
{
	kthd_parm_t parm = (kthd_parm_t)kalloc(sizeof(struct __kthd_parm_t));
	assert(parm);
	parm->fn = fn;
	parm->c = c;
	struct sl_thd *s = sl_thd_alloc(__lwt_kthd_entry,parm);
	union sched_param spl = {.c = {.type = SCHEDP_PRIO, .value = 10}};
	sl_thd_param_set(s, spl.v);
	return (s != NULL)-1;
}

int lwt_snd_thd(lwt_chan_t c, lwt_t sending)
{
	assert(sending != lwt_head);
	sending->status = LWT_BLOCKED;
	gcounter.runable_counter--;
	gcounter.blocked_counter++;
	return lwt_snd(c,sending);
}

lwt_t lwt_rcv_thd(lwt_chan_t c)
{
	lwt_t lwt = lwt_rcv(c);
	lwt->tid = lwt_head->tid;
	ps_list_rem_d(lwt);
	ps_list_add_d(ps_list_prev_d(lwt_head),lwt);
	lwt->status = LWT_ACTIVE;
	gcounter.blocked_counter--;
	gcounter.runable_counter++;
	return lwt;
}



