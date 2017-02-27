#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
//#include"lwt_dispatch.h"
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




/*
 * pool lwt  head
 */
lwt_t pool_head;

/*
 * tail pool of lwt
 */
lwt_t pool_tail;

/*
 * the pointer of the free pool
 */
void *pool;

/*
 * the number of free space in the pool
 */
uchar rest_pool;

__attribute__((constructor())) void 
__init()  
{  
	__init_counter();
	__init_pool();
	__init_thread_head();

}  	

void
__init_counter()
{
    gcounter.lwt_count = 0;
	gcounter.runable_counter = 0;
	gcounter.blocked_counter = 0;
	gcounter.died_counter = 0;
	gcounter.avail_counter = 0;
	gcounter.nchan_counter = 0;	
	gcounter.nsnding_counter = 0;
	gcounter.nrcving_counter = 0;
}

void
__init_thread_head()
{

	lwt_head = __lwt_stack_get();
	ps_list_init_d(lwt_head);

	lwt_head->ip = (ulong)0;
	lwt_head->sp = (ulong)NULL;

	lwt_head->bsp = (ulong)NULL;
	lwt_head->id = gcounter.lwt_count++;

	lwt_head->ifrecycled = 0;
	lwt_curr = lwt_head;
	lwt_curr->status = LWT_ACTIVE;
	gcounter.runable_counter++;
	lwt_tail = lwt_head;

}

void
__init_pool()
{
	rest_pool = 20;
	pool =  malloc((sizeof(struct _lwt_t ) + STACK_SIZE) * rest_pool);
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
//	lwt_new->stack = __lwt_stack_get();
//	lwt_new->sp = lwt_new->stack->bsp + STACK_SIZE - 4; 		
	lwt_new->sp = lwt_new->bsp + STACK_SIZE - 4; 		
	lwt_new->status = LWT_ACTIVE;
	lwt_new->joiner = LWT_NULL; 
	lwt_new->fn = fn;
	lwt_new->data = data;
	lwt_new->ifrecycled = 0;
	gcounter.runable_counter++;
	//lwt_des = lwt_new;
	
	ps_list_add_d(lwt_tail,lwt_new);

	lwt_tail = lwt_new;
	return lwt_new;
}

void*
lwt_join(lwt_t lwt)
{	
	void *temp_data;

	assert(lwt);

	lwt_curr->joiner = lwt;
	lwt->target = lwt_curr;

	if(lwt->status != LWT_DEAD)
	{
		lwt_curr->status = LWT_BLOCKED;
		gcounter.blocked_counter++;
		gcounter.runable_counter--;
		lwt_yield(lwt);
	}
	if(!lwt->ifrecycled)
	{
		temp_data = lwt->return_val;
		if(lwt_tail == lwt)	
			lwt_tail = ps_list_prev_d(lwt_tail);
		ps_list_rem_d(lwt);
		__lwt_stack_return(lwt);
		gcounter.died_counter--;
	}

	return temp_data;

}

void
lwt_die(void *data)
{	

	/*
	 * if the current thread is the joiner of a specific thread,
	 * set this blocked thread active
	 */

	if (lwt_curr->target != NULL)
	{
		if(lwt_curr->target->status == LWT_BLOCKED)
		{
			gcounter.blocked_counter--;
			lwt_curr->target->status = LWT_ACTIVE;
			gcounter.runable_counter++;
		}
	}
	
	//mark the current thread state as DEAD, free the stack
	lwt_curr->status = LWT_DEAD;

	lwt_curr->return_val = (void *)data;

	gcounter.runable_counter--;

	gcounter.died_counter++;

	if(lwt_curr->target != LWT_NULL && lwt_curr->target->status == LWT_ACTIVE)
	{
		lwt_yield(lwt_curr->target);
	}	
	else
	{
		__lwt_schedule();
	}
		//lwt_yield(lwt_head);

//	lwt_yield(LWT_NULL);
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
	//	lwt->status = LWT_ACTIVE;
		lwt_des = lwt;
				lwt_t tmp_curr = lwt_curr;
		lwt_curr = lwt_des;
		__lwt_dispatch(/*(struct lwt_context *)*/tmp_curr,/*(struct lwt_context *)*/lwt_des);
	}
	return -1;
}

lwt_t
lwt_current(void)
{
	return lwt_curr;
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
		
	void *return_val = lwt_curr->fn(lwt_curr->data);
	lwt_die(return_val);

}

void
__lwt_schedule(void)
{
	lwt_t temp;

	//prevent directly yield()
	if (lwt_info(LWT_INFO_NTHD_RUNNABLE) == 0)
	{
		return;
	}
		
	if(lwt_curr != lwt_tail)
	{		
		ps_list_rem_d(lwt_curr);
		ps_list_add_d(lwt_tail,lwt_curr);
		lwt_tail = lwt_curr;
	}                              	

	temp = ps_list_next_d(lwt_tail);
	while(temp->status != LWT_ACTIVE)
	{
		temp = ps_list_next_d(temp);
	}

	if (temp != lwt_curr)
	{
		lwt_des = temp;

		lwt_t tmp_curr = lwt_curr;
		lwt_curr = lwt_des;
		__lwt_dispatch(/*(struct lwt_context *)*/tmp_curr, /*(struct lwt_context *)*/lwt_des);

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
		if(rest_pool > 0)
		{	
			pool_head = __get_space_from_pool(sizeof(struct _lwt_t));
			pool_head->bsp = (ulong) __get_space_from_pool(STACK_SIZE);
			rest_pool--;
		}
		else
		{
			pool_head = malloc(sizeof(struct _lwt_t));
			pool_head->bsp = (ulong) (malloc(STACK_SIZE)) ;	//be used to free the stack
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
	tmp->ifrecycled = 1;
	
	if(gcounter.avail_counter == 0)
	{
		ps_list_init_d(tmp);
		pool_head = tmp;
		pool_tail = tmp;
	}
	else
	{
		ps_list_add_d(pool_tail,tmp);		
		pool_tail = tmp;
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


