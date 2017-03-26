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
lwt_t pool_head;

/*pool channel  head*/
lwt_chan_t pool_chan_head;

/*pool clist head*/
clist_t pool_clist_head;
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
	gcounter.runable_counter++;

}

void
__init_pool()
{
	rest_pool = 5 * (sizeof(struct _lwt_t ) + STACK_SIZE + sizeof(struct lwt_channel));
	pool =  malloc(rest_pool);
}

lwt_t 
lwt_create(lwt_fn_t fn, void *data)
{

	lwt_t lwt_new;

	assert(fn);
	
	//malloc the new thread and stack


	lwt_new = __lwt_stack_get();
	lwt_new->id = gcounter.lwt_count++;
	lwt_new->ip = (ulong) (&__lwt_start);
	lwt_new->sp = lwt_new->bsp + STACK_SIZE - 4; 		
	lwt_new->status = LWT_ACTIVE;
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

	lwt_head->joiner = lwt;
	lwt->target = lwt_head;

	if(lwt->status == LWT_ACTIVE)
	{
		lwt_head->status = LWT_BLOCKED;
		gcounter.blocked_counter++;
		gcounter.runable_counter--;
		lwt_yield(lwt);
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

	lwt_head->status = LWT_DEAD;
	lwt_head->return_val = (void *)data;
	gcounter.runable_counter--;
	gcounter.died_counter++;

	if(lwt_head->target != LWT_NULL && lwt_head->target->status == LWT_BLOCKED)
	{
		gcounter.blocked_counter--;
		lwt_head->target->status = LWT_ACTIVE;
		gcounter.runable_counter++;
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
		lwt_t tmp_curr = lwt_head;
		lwt_head = lwt;
		__lwt_dispatch(/*(struct lwt_context *)*/tmp_curr,/*(struct lwt_context *)*/lwt_head);
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

	temp = lwt_head;
	lwt_head = ps_list_next_d(lwt_head);
	while(lwt_head->status != LWT_ACTIVE)
	{
		lwt_head = ps_list_next_d(lwt_head);
	}

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

void 
*__lwt_stack_get()
{

	if(gcounter.avail_counter == 0)
	{
		uint size = sizeof(struct _lwt_t) + STACK_SIZE;
		if(rest_pool >= size)
		{	
			pool_head = __get_space_from_pool(size);
			pool_head->bsp = (ulong)pool_head + size - STACK_SIZE;
			rest_pool -= size;
		}
		else
		{
			pool_head = malloc(size);
			pool_head->bsp = (ulong)pool_head + size - STACK_SIZE;
		}

		return pool_head;
	}
	else
	{
		lwt_t tmp = pool_head;		
		pool_head = ps_list_next_d(pool_head);
		ps_list_rem_d(tmp);
		gcounter.avail_counter--;
		return tmp;
	}	
}

void 
__lwt_stack_return(void *lwt)
{
	assert(lwt);

	lwt_t tmp = lwt;
	
	if(gcounter.avail_counter == 0)
	{
		ps_list_init_d(tmp);
		pool_head = tmp;
	}
	else
	{
		ps_list_add_d(ps_list_prev_d(pool_head),tmp);		
	}

	gcounter.avail_counter++;
}

void
*__get_space_from_pool(int space_size)
{
	assert(rest_pool > 0);

	void *tmp = pool;
	pool = pool + space_size;
	return (void *)tmp;
}

/************************************************************/
/*                   ring_buffer							*/
/************************************************************/
//pass in the data_buffer, and the size
void rb_init(ring_buffer* rb, int size) 
{
	assert(size >= 0);
	rb->size = size;
	rb->start = 0;
	rb->end = 0;
	rb->num = 0;
	rb->data = (void **)calloc(rb->size, sizeof(void *));
}

int rbIsFull(ring_buffer* rb) 
{
		return (rb->num) == rb->size;
}	

int rbIsEmpty(ring_buffer* rb) 
{
	return rb->num == 0; 
}

void rb_add(ring_buffer* rb, void* data) 
{
	printf("end = %d\n",rb->end);
	rb->data[rb->end] = data;
	rb->end = (rb->end + 1)%rb->size;
	rb->num++;
	return;
}

void* rb_get(ring_buffer* rb) 
{
	printf("start = %d\n",rb->start);
	void* temp = rb->data[rb->start];
	rb->start = (rb->start + 1)%rb->size;
	rb->num--;
	return temp;
}

void clist_add(clist_t new_clist,lwt_chan_t c)
{

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
void clist_rem(lwt_chan_t c)
{
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
/* 
 * Currently assume that sz is always 0. 
 * This function uses malloc to allocate 
 * the memory for the channel. 
 */

lwt_chan_t 
lwt_chan(int sz)
{
	lwt_chan_t new = (lwt_chan_t)__channel_get();
	rb_init(&new->data_buffer,sz);
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
	return;
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

	gcounter.nsnding_counter++;
	gcounter.runable_counter--;
	if(c->data_buffer.size == 0)
	{
		//if there is node that is new added without filling the data,fill it 
		clist_t new_clist = (clist_t)__channel_get();
		new_clist->thd = lwt_head;
		new_clist->data = data;					
		clist_add(new_clist,c);
		//block current thread
		lwt_head->status = LWT_WAITING;				
		while (c->rcv_thd->status == LWT_ACTIVE)//blocked == 0) 
		{
			lwt_yield(LWT_NULL);								
		}
	}
	else
	{
		while(rbIsFull(&c->data_buffer)) 
		{
			lwt_yield(LWT_NULL);							
		}	
		rb_add(&c->data_buffer, data);
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
		while (c->snd_thds == NULL) 
		{
			lwt_yield(LWT_NULL);
						
		}
		temp = c->snd_thds->data;
		clist_rem(c);
	}
	else
	{
		//if empty, block rcv, remove c from ready queue
		while (rbIsEmpty(&c->data_buffer)) 
		{
			DEBUG();
			lwt_yield(LWT_NULL);
		}
		temp = rb_get(&c->data_buffer);

	}
	gcounter.nrcving_counter--;
	gcounter.runable_counter++;
	c->rcv_thd->status = LWT_ACTIVE;
	//remove the event
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
	
	lwt_t lwt = lwt_create((lwt_fn_t)fn,(void*)c);
	
	c->snd_cnt++;

	return lwt;
}

void 
*__channel_get()
{

	if(gcounter.avail_chan_counter == 0)
	{
		uint size = sizeof(struct lwt_channel);
		if(rest_pool >= size)
		{	
			pool_chan_head = __get_space_from_pool(size);
			rest_pool -= size;
		}
		else
		{
			pool_chan_head = malloc(size);
		}
		return pool_chan_head;
	}
	else
	{
		lwt_chan_t tmp = pool_chan_head;		
		pool_chan_head = ps_list_next_d(pool_chan_head);
		ps_list_rem_d(tmp);
		gcounter.avail_chan_counter--;
		return tmp;
	}	
}

void 
__channel_return(void *chan)
{
	assert(chan);

	lwt_chan_t tmp = chan;
	if(gcounter.avail_chan_counter == 0)
	{
		ps_list_init_d(tmp);
		pool_chan_head = tmp;
	}
	else
	{
		ps_list_add_d(ps_list_prev_d(pool_chan_head),tmp);		
	}

	gcounter.avail_chan_counter++;
}

void 
*__clist_get()
{	
	if(gcounter.avail_clist_counter == 0)
	{
		uint size = sizeof(struct clist_head);
		if(rest_pool >= size)
		{	
			pool_clist_head = __get_space_from_pool(size);
			rest_pool -= size;
		}
		else
		{
			pool_clist_head = malloc(size);															
		}
		return pool_chan_head;
	}
	else
	{
		clist_t tmp = pool_clist_head;		
		pool_clist_head = ps_list_next_d(pool_clist_head);
		ps_list_rem_d(tmp);
		gcounter.avail_clist_counter--;
		return tmp;										
	}	
}

void 
__clist_return(void *clist)
{
	assert(clist);
	clist_t tmp = clist;
	if(gcounter.avail_clist_counter == 0)
	{
		ps_list_init_d(tmp);
		pool_clist_head = tmp;
	}
	else
	{
		ps_list_add_d(ps_list_prev_d(pool_clist_head),tmp);				
	}
	gcounter.avail_clist_counter++;
}

lwt_cgrp_t lwt_cgrp(void)
{
	lwt_cgrp_t new = NULL;
	new->n_chan = 0;
	new->events = NULL;
	return new;

}

int lwt_cgrp_free(lwt_cgrp_t cgrp)
{
	if (cgrp->events = NULL)
	{
    	free(cgrp);
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
	if (rbIsEmpty(&c->data_buffer) == 0) 
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
