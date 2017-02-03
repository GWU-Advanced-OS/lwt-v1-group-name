#include<stdio.h>
#include<stdlib.h>
#include"lwt_dispatch.h"
#include"lwt.h"
void __lwt_start();
void __lwt_schedule(void);
void enqueue(lwt_t lwt);


lwt_t lwt_create(lwt_fn_t fn, void *data)
{

	//malloc the new thread and stack
	lwt_new = (lwt_t)malloc(sizeof(struct _lwt_t));
	lwt_counter++;
	lwt_new->id = lwt_counter;
	lwt_new->ip = &__lwt_start;
	lwt_new->sp = malloc(STACK_SIZE) + STACK_SIZE - 4; 		//for free stack
	lwt_new->state = LWT_ACTIVE;
	lwt_new->parent_id = lwt_curr->id;

	lwt_new->joiner = LWT_NULL; 
	lwt_new->fn = fn;
	lwt_new->data = data;
	runable_counter++;

	/*
	 * if current thread is NULL, make the new thread be the head
	 */
	if (lwt_head == LWT_NULL)
	{
		lwt_head = lwt_new;
		lwt_curr = LWT_NULL;
		lwt_tail = lwt_head;
	}
	else
	{
		enqueue(lwt_new);	
	}

	return lwt_new;
}

void *lwt_join(lwt_t lwt)
{
	void *temp_data;
	if(lwt_curr->status == LWT_DEAD)
	{
		lwt->target = LWT_NULL;
	}
	else
	{
		lwt_curr->joiner = lwt;
		lwt->target = lwt_curr;
	}

	if(lwt->status == LWT_DEAD)
	{
		died_counter--;
		free(lwt->sp - STACK_SIZE +4);
		lwt->pre->next = lwt->next;
		if(lwt->next == NULL)
		{
			lwt_tail = lwt->pre;
		}
		else
		{
			lwt_next->pre = lwt->pre;
		}
		temp_data = lwt->return_val;
		free(lwt);
		return temp;
	}


	if(lwt->target != LWT_NULL)
	{
		lwt_curr->status = LWT_BLOCKED;
		blocked_counter++;
		runable_counter--;
	}
	lwt_yield(lwt);
	//free stack
	free(lwt->sp - STACK_SIZE +4);
	lwt->pre->next = lwt->next;
	//remove lwt from linked list
	if (lwt->next == NULL)
		lwt_tail = lwt->pre;
	else
		lwt->next->pre = lwt->pre;
	//free lwt
	temp_data = lwt->return_val;
	free(lwt);
	return temp_data;

}

void lwt_die(void *data)
{	
	/*
	 * if the current thread is the joiner of a specific thread,
	 * set this blocked thread active
	 */

	if (lwt_curr->target != NULL)
	{
		lwt_curr->target->status = LWT_ACTIVE;
		runable_counter++;
	}
	
	//mark the current thread state as DEAD, free the stack
	lwt_curr->status = LWT_DEAD;

	lwt_curr->return_val = (void *)data;

	runable_counter--;

	died_counter++;

	lwt_yield(LWT_NULL);
}

int lwt_yield(lwt_t lwt)
{
	if(lwt == NULL)
		__lwt_schedule();
	else if (lwt->state == LWT_ACTIVE)
	{
		lwt_des = lwt;
		struct lwt_context curr,next;
		memcpy(lwt_curr,&curr,sizeof(struct lwt_context));
		memcpy(lwt_des,&next,sizeof(struct lwt_context));
		__lwt_dispatch(&curr,&next);
	}
}

lwt_t lwt_current(void)
{
	return lwt_curr;
}

/*
 * gets the thread id of a specified thread.
 * returns -1 if the thread not exists
 */
int lwt_id(lwt_t lwt)
{
	if (!lwt)
		return -1;
	
	return lwt->id;
}

/*
 * get the number of threads that are either runnable,
 * blocked (i.e. that are joining, on a lwt that hasn't died),
 *  or that have died
 */
int lwt_info(lwt_info_t t)
{
	switch (t) {
		case LWT_INFO_NTHD_RUNNABLE:
			return runable_counter;
		case LWT_INFO_NTHD_BLOCKED:
			return blocked_counter;
		case LWT_INFO_NTHD_ZOMBIES:
		default:
			return died_counter;							
	}
}


void __lwt_start()
{
	lwt_curr = lwt_des;
	void *return_val = lwt_curr->fn(lwt_curr->data);
	lwt_die(return_val);
}

void __lwt_schedule(void);
{
	lwt_t temp;
	
	//prevent directly yield()
	if (lwt_info(LWT_INFO_NTHD_RUNNABLE) == 0)
	{
		return;
	}
	
	//get next ACTIVE thread
	if (lwt_curr != LWT_NULL && lwt_curr->next != LWT_NULL) 
	{		
		temp = lwt_curr->next;
	} 
	else 
	{ 
		temp = lwt_head;
	}
	
	while (temp->state != LWT_ACTIVE)
	{
		if (temp->next != NULL)
		{
			temp = temp->next;
		}
		else 
		{											 													temp = head_tcb;
			temp = lwt_head;
		}
	}
	
	if (temp != lwt_curr)
	{
		//printf("/////Yielding to %d\n", temp->id);
		lwt_des = temp;
		struct lwt_context curr,next;
		memcpy(lwt_curr,&curr,sizeof(struct lwt_context));
    	memcpy(lwt_des,&next,sizeof(struct lwt_context));
		__lwt_dispatch(&curr, &next);
	}
	return;	
}

void enqueue(lwt_t lwt)
{
	lwt->prev = lwt_tail;
	lwt_tail->next = lwt;
	lwt_tail = lwt;
}
