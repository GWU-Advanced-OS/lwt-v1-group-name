#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include"lwt.h"


void __lwt_start();
void __lwt_schedule(void);
static inline void __lwt_dispatch(lwt_t curr, lwt_t next);	
void *__lwt_stack_get(void);
void __lwt_stack_return(void *stk);
void __init_counter(void);
void __init_thread_head(void);
void __init_pool(void);
void* __get_space_from_pool(int space_size);
void* __channel_get();
void __channel_return(void *chan);
void* __clist_get();
void __clist_return(void *clist);





/*pool lwt  head*/
struct ps_list_head pool_head;

/*pool channel  head*/
struct ps_list_head pool_chan_head;

/*pool clist head*/
struct ps_list_head pool_clist_head;

/*pool cgrp head*/
struct ps_list_head pool_cgrp_head;

/*the pointer of the free pool*/
void *pool;

/*the number of free space in the pool*/
ulong rest_pool;

__attribute__((constructor())) void 
__init()  
{  
	__init_pool();
	__init_thread_head();

}  	


void
__init_thread_head()
{

	lwt_head = __lwt_stack_get();
	ps_list_init_d(lwt_head);
	lwt_head->ip = (ulong)0;
	lwt_head->sp = (ulong)NULL;
	lwt_head->id = gcounter.lwt_count++;
	lwt_head->status = LWT_ACTIVE;
	lwt_curr = lwt_head;
	gcounter.runable_counter++;

}

void
__init_pool()
{
	rest_pool = 5 * (sizeof(struct _lwt_t ) + STACK_SIZE + sizeof(struct lwt_channel) + sizeof(struct cgroup));
	ps_list_head_init(&pool_head);
	ps_list_head_init(&pool_chan_head);
	ps_list_head_init(&pool_clist_head);
	ps_list_head_init(&pool_cgrp_head);	
	pool =  malloc(rest_pool);
	if(pool == NULL)
		assert(0);
}

void 
*__lwt_stack_get()
{
	lwt_t lwt;
	if(gcounter.avail_counter == 0)
	{
		uint size = sizeof(struct _lwt_t) + STACK_SIZE;
		if(rest_pool >= size)
		{	
			lwt = __get_space_from_pool(size);
			rest_pool -= size;
		}
		else
		{
			lwt = malloc(size);
			if(lwt == NULL)
				assert(0);
		}
	}
	else
	{
//		lwt = ps_list_head_last(&pool_head, struct _lwt_t, list);
		lwt = (lwt_t)((char *)pool_head.l.p - sizeof(struct _lwt_t) + sizeof(struct ps_list));
//		printf("lwt adrr %ld, cal adrr %ld, struct size %d\n",(ulong)lwt,(ulong)temp,sizeof(struct _lwt_t));		
//		void *a=1;
//		char *b;
//		printf("void %ld, char %ld\n",pool_head.l.p, (char *)pool_head.l.p);
		ps_list_rem_d(lwt);
		gcounter.avail_counter--;
	}	
	return lwt;
}

void 
__lwt_stack_return(void *lwt)
{
	assert(lwt);
	ps_list_head_add_d(&pool_head,(lwt_t)lwt);
	gcounter.avail_counter++;
}

void
*__get_space_from_pool(int space_size)
{
	assert(rest_pool > 0);

	void *tmp = pool;
	pool = pool + space_size;
	return tmp;
}

lwt_t 
lwt_create(lwt_fn_t fn, void *data, lwt_flags_t flags)
{

	lwt_t lwt_new;

	assert(fn);
	
	//malloc the new thread and stack


	lwt_new = __lwt_stack_get();
	lwt_new->id = gcounter.lwt_count++;
	lwt_new->ip = (ulong) (&__lwt_start);
	lwt_new->sp = (ulong)lwt_new + sizeof(struct _lwt_t) + STACK_SIZE - 4; 		
	lwt_new->status = LWT_ACTIVE;
	lwt_new->lwt_nojoin = flags;
	lwt_new->fn = fn;
	lwt_new->data = data;
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
		lwt_yield(lwt);
		/*lwt_chan_t c = lwt_chan(0);
		c->snd_cnt = 1;
		lwt_head->data = c;
		lwt_rcv(c);
		lwt_chan_deref(c);*/
	}
	temp_data = lwt->return_val;
	ps_list_rem_d(lwt);
	__lwt_stack_return(lwt);
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
		lwt_head = ps_list_next_d(lwt_head);
		ps_list_rem_d(lwt_curr);
		__lwt_stack_return(lwt_curr);
	}
	else
	{
		lwt_head->status = LWT_DEAD;
		gcounter.died_counter++;
	}

	__lwt_schedule();	
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
		lwt_t tmp_curr = lwt_curr;
		lwt_head = lwt;
		lwt_curr = lwt_head;
		__lwt_dispatch(tmp_curr,lwt_head);
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

	temp = lwt_curr;
	if(lwt_curr == lwt_head)
		lwt_head = ps_list_next_d(lwt_head);
	while(lwt_head->status != LWT_ACTIVE)
	{
		lwt_head = ps_list_next_d(lwt_head);
	}

	if (temp != lwt_head)
	{
		lwt_curr = lwt_head;
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
	lwt_chan_t new = (lwt_chan_t)__channel_get();
	assert(sz >= 0);
	new->data_buffer.size = sz;
	new->data_buffer.start = 0;
	new->data_buffer.end = 0;
	new->data_buffer.num = 0;
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
	if(c->snd_cnt > 0)
	{
		c->snd_cnt--;
	}
	else if (c->rcv_thd->status == LWT_ACTIVE) 
	{
		gcounter.nchan_counter--;
		c->rcv_thd = NULL; 	//will also set "rcv_thd" as NULL
		__channel_return(c);
		return;
	}
	else
	{
		assert(0);
	}
}

int 
lwt_snd(lwt_chan_t c, void *data)
{
	//data being NULL or rcv being null is illegal
	assert(data && c);

	if (c->rcv_thd == NULL)
   	{
		return -1;
	}

	gcounter.nsnding_counter++;
	gcounter.runable_counter--;
	if(c->data_buffer.size == 0)
	{

		//block current thread
		while (c->rcv_thd->status == LWT_ACTIVE)//blocked == 0) 
		{
			lwt_yield(LWT_NULL);								
		}
		//if there is node that is new added without filling the data,fill it 
		clist_t new_clist = (clist_t)__clist_get();
		new_clist->thd = lwt_head;
		new_clist->data = data;					
//		ps_list_head_add_d(&c->snd_head, new_clist);
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
		//add the event
		if (c->iscgrp) 
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
		while(c->data_buffer.num == c->data_buffer.size) 
		{
			lwt_yield(LWT_NULL);							
		}	
		c->data_buffer.data[c->data_buffer.end] = data;
		c->data_buffer.end = (c->data_buffer.end + 1)%c->data_buffer.size;
		c->data_buffer.num++;
	}

	c->rcv_thd->status = LWT_ACTIVE;
	gcounter.nsnding_counter--;
	gcounter.runable_counter++;
	return 0;
}

void 
*lwt_rcv(lwt_chan_t c)
{
	assert(c != NULL);
	void* temp;
	if (c->rcv_thd != lwt_head) 
	{
		return NULL;
	}

	c->rcv_thd->status = LWT_WAITING;//blocked = 1;
	gcounter.nrcving_counter++;
	gcounter.runable_counter--;
	if(c->data_buffer.size == 0)
	{
		//if snd_thds is null, block the reciever to wait for sender
		while (c->snd_thds == NULL)//ps_list_head_empty(&c->snd_head)) 
		{
			lwt_yield(LWT_NULL);						
		}

		temp = c->snd_thds->data;
		if(ps_list_singleton_d(c->snd_thds))
		{
			__clist_return(c->snd_thds);
			c->snd_thds = NULL;														
		}
		else
		{
			clist_t tmp = c->snd_thds;
			c->snd_thds = ps_list_next_d(tmp);
			ps_list_rem_d(tmp);
			__clist_return(tmp);																								
		}
	}
	else
	{
		//if empty, block rcv, remove c from ready queue
		while(c->data_buffer.num == 0) 
		{
			lwt_yield(LWT_NULL);
		}
		temp = c->data_buffer.data[c->data_buffer.start];
		c->data_buffer.start = (c->data_buffer.start + 1)%c->data_buffer.size;
		c->data_buffer.num--;

		if (c->cgrp != NULL) 
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
lwt_create_chan(lwt_chan_fn_t fn, lwt_chan_t c)
{
	assert(fn);	
	assert(c);
	
	lwt_t lwt = lwt_create((lwt_fn_t)fn,(void*)c,0);
	
	c->snd_cnt++;

	return lwt;
}

void 
*__channel_get()
{
	lwt_chan_t c;
	if(gcounter.avail_chan_counter == 0)
	{
		uint size = sizeof(struct lwt_channel);
		if(rest_pool >= size)
		{	
			c = __get_space_from_pool(size);
			rest_pool -= size;
		}
		else
		{
			c = malloc(size);
			if(c == NULL)
				assert(c);
		}
	}
	else
	{
		//c = ps_list_head_last(&pool_chan_head, struct lwt_channel, list);		
		c = (lwt_chan_t)((char *)pool_chan_head.l.p - sizeof(struct lwt_channel) + sizeof(struct ps_list));
		ps_list_rem_d(c);
		gcounter.avail_chan_counter--;
	}	
	return c;
}

void 
__channel_return(void *chan)
{
	assert(chan);
	ps_list_head_add_d(&pool_chan_head, (lwt_chan_t)chan);
	gcounter.avail_chan_counter++;
}

void 
*__clist_get()
{
	clist_t c;	
	if(gcounter.avail_clist_counter == 0)
	{
		uint size = sizeof(struct clist_head);
		if(rest_pool >= size)
		{	
			c = __get_space_from_pool(size);
			rest_pool -= size;
		}
		else
		{
			c = malloc(size);															
			if(c == NULL)
				assert(0);
		}
	}
	else
	{
		//c = ps_list_head_last(&pool_clist_head, struct clist_head, list);		
		c = (clist_t)((char *)pool_clist_head.l.p - sizeof(struct clist_head) + sizeof(struct ps_list));
		ps_list_rem_d(c);
		gcounter.avail_clist_counter--;
	}	
	return c;
}

void 
__clist_return(void *clist)
{
	assert(clist);
	ps_list_head_add_d(&pool_clist_head, (clist_t)clist);
	gcounter.avail_clist_counter++;
}

void 
*__cgrp_get()
{
	lwt_cgrp_t c;	
	if(gcounter.avail_cgrp_counter == 0)
	{
		uint size = sizeof(struct cgroup);
		if(rest_pool >= size)
		{	
			c = __get_space_from_pool(size);
			rest_pool -= size;
		}
		else
		{
			c = malloc(size);															
			if(c == NULL)
				assert(0);
		}
	}
	else
	{
		//c = ps_list_head_last(&pool_cgrp_head, struct cgroup, list);		
		c = (lwt_cgrp_t)((char *)pool_cgrp_head.l.p - sizeof(struct cgroup) + sizeof(struct ps_list));
		ps_list_rem_d(c);
		gcounter.avail_cgrp_counter--;
	}	
	return c;
}

void 
__cgrp_return(void *cgrp)
{
	assert(cgrp);
	ps_list_head_add_d(&pool_cgrp_head, (lwt_cgrp_t)cgrp);
	gcounter.avail_cgrp_counter++;
}

lwt_cgrp_t 
lwt_cgrp(void)
{
	lwt_cgrp_t new;
	new = __cgrp_get();
	new->n_chan = 0;
	new->events = NULL;
	return new;

}

int 
lwt_cgrp_free(lwt_cgrp_t cgrp)
{
	if (cgrp->events = NULL)
	{
		__cgrp_return(cgrp);
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

}

int lwt_cgrp_rem(lwt_cgrp_t cgrp, lwt_chan_t c)
{
	if (c->data_buffer.num != 0) 
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
	while (cgrp->events == NULL) 
	{
		lwt_yield(NULL);			    
	}
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
