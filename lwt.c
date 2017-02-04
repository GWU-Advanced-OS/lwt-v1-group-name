#include<stdio.h>
#include<stdlib.h>
#include"lwt_dispatch.h"
#include"lwt.h"


void __lwt_start();
void __lwt_schedule(void);
void enqueue(lwt_t lwt);

/*
 * auto-increment counter for thread
 */
uint lwt_counter = 0;	

/*
 * counter for runable thread
 */
uint runable_counter;

/*
 * counter for blocked thread
 */
uint blocked_counter;

/*
 * counter for died thread
 */
uint died_counter;

/*
 * head of the queue of  thread
 */
lwt_t lwt_head;

/*
 *tail of the queue of thread
 */
lwt_t lwt_tail;

/*
 * current thread
 */
lwt_t lwt_curr;

/*
 * the destination thread which is going to be operated
 */
lwt_t lwt_des;


lwt_t lwt_create(lwt_fn_t fn, void *data)
{

	lwt_t lwt_new,freak;
	/*
	 * if current thread is NULL, make the new thread be the head
	 */
	if (lwt_head == LWT_NULL)
	{
		lwt_head = (lwt_t)malloc(sizeof(struct _lwt_t));
		lwt_head->ip = 0;
		lwt_head->sp = NULL;
		lwt_head->bsp = NULL;
		lwt_head->id = lwt_counter++;
		lwt_curr = lwt_head;
		lwt_curr->status = LWT_ACTIVE;
		runable_counter++;
		lwt_tail = lwt_head;
	}
	
	//malloc the new thread and stack
//	freak = (lwt_t)malloc(sizeof(struct _lwt_t));
	if(lwt_head == lwt_curr)
		printf("head == curr\n");
	else
		printf("head != curr \n");
	printf("create curr_id = %d\n",lwt_curr->id);

	lwt_new = (lwt_t)malloc(sizeof(struct _lwt_t));
//	free(freak);

	lwt_new->id = lwt_counter++;
	printf("create curr_id = %d\n",lwt_curr->id);
	lwt_new->ip = (ulong) (&__lwt_start);

	lwt_new->bsp = (ulong) (malloc(STACK_SIZE)) ;	//be used to free the stack

	lwt_new->sp = lwt_new->bsp + STACK_SIZE - 4; 		

	lwt_new->status = LWT_ACTIVE;


	lwt_new->joiner = LWT_NULL; 
	lwt_new->fn = fn;
	lwt_new->data = data;
	runable_counter++;

	lwt_des = lwt_new;


	printf("create curr_id = %d\n",lwt_curr->id);
	printf("des des_id = %d\n",lwt_des->id);
	
	enqueue(lwt_new);	

	return lwt_new;
}

void *lwt_join(lwt_t lwt)
{
#ifdef __DEBUG
	DEBUG();
#endif
	printf("join curr_id = %d\n",lwt_curr->id);

	void *temp_data;
/*	if(lwt_curr->status == LWT_DEAD)
	{
		lwt->target = LWT_NULL;
	}
	else
	{*/
/*	if(lwt_curr->status != LWT_ACTIVE)
	{
		lwt_head->joiner = lwt;
		lwt->target = lwt_head;
	}
	else
	{*/
		lwt_curr->joiner = lwt;
		lwt->target = lwt_curr;
//	}

	if(lwt->status == LWT_DEAD)
	{
		died_counter--;
	}
	else
	{
		printf("lwt_curr id = %d\n",lwt_curr->id);
		printf("lwt_curr status = %d\n",lwt_curr->status);
		printf("lwt id = %d\n",lwt->id);
		printf("lwt status = %d\n",lwt->status);
		lwt_curr->status = LWT_BLOCKED;
		blocked_counter++;
		runable_counter--;

		lwt_yield(lwt);
	}

	//free stack
	free(lwt->bsp);
	lwt->prev->next = lwt->next;

	//remove lwt from linked list
	if (lwt->next == NULL)
		lwt_tail = lwt->prev;
	else
		lwt->next->prev = lwt->prev;
	died_counter--;
	//free lwt
	temp_data = lwt->return_val;
	free(lwt);
	return temp_data;

}

void lwt_die(void *data)
{	
#ifdef __DEBUG
	DEBUG();
#endif
	/*
	 * if the current thread is the joiner of a specific thread,
	 * set this blocked thread active
	 */

	if (lwt_curr->target != NULL)
	{
		if(lwt_curr->target->status == LWT_BLOCKED)
		{
			blocked_counter--;
			lwt_curr->target->status = LWT_ACTIVE;
			runable_counter++;
		}
	}
	
	//mark the current thread state as DEAD, free the stack
	lwt_curr->status = LWT_DEAD;

	lwt_curr->return_val = (void *)data;

	runable_counter--;

	died_counter++;
	#ifdef __DEBUG
	DEBUG();
#endif
	lwt_yield(LWT_NULL);
}

int lwt_yield(lwt_t lwt)
{
#ifdef __DEBUG
	DEBUG();	
#endif
	if(lwt == NULL)
	{
		__lwt_schedule();
	
	}
	else if (lwt->status == LWT_ACTIVE)
	{
		lwt_des = lwt;
		struct lwt_context *curr,*next;

		curr = (struct lwt_context *) lwt_curr;
		next = (struct lwt_context *) lwt_des;
		//memcpy(&curr,lwt_curr,sizeof(struct lwt_context));
		//memcpy(&next,lwt_des,sizeof(struct lwt_context));
		
#ifdef __DEBUG
	DEBUG();
	printf("cur id = %d, ip = %d, sp = %d\n",lwt_curr->id,curr->ip,curr->sp);
	printf("des id = %d, ip = %d, sp = %d\n",lwt_des->id,next->ip,next->sp);
#endif

		__lwt_dispatch(curr,next);
	}
	else 
	{
		printf("status = %d\n",lwt->status);
		printf("head status = %d\n",lwt_head->status);
	}
	return -1;
}

lwt_t lwt_current(void)
{
#ifdef __DEBUG
	DEBUG();
#endif
	return lwt_curr;
}

/*
 * gets the thread id of a specified thread.
 * returns -1 if the thread not exists
 */
int lwt_id(lwt_t lwt)
{
#ifdef __DEBUG
	DEBUG();
#endif
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
#ifdef __DEBUG
	DEBUG();
#endif
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
#ifdef __DEBUG
	DEBUG();
#endif
	lwt_curr = lwt_des;
	printf("current id = %d,ip = %d, sp = %d\n",lwt_curr->id,lwt_curr->ip,lwt_curr->sp);
	
	void *return_val = lwt_curr->fn(lwt_curr->data);
	lwt_die(return_val);

#ifdef __DEBUG
	DEBUG();
#endif
}

void __lwt_schedule(void)
{
	lwt_t temp;

#ifdef __DEBUG
	printf("runable = %d\n",runable_counter);
#endif	
	//prevent directly yield()
	if (lwt_info(LWT_INFO_NTHD_RUNNABLE) == 0)
	{
		return;
	}
	
	//get next ACTIVE thread
	if (lwt_curr->next != LWT_NULL) 
	{	
		temp = lwt_curr->next;
	} 
	else 
	{ 
		temp = lwt_head;
	}
	
	while (temp->status != LWT_ACTIVE)
	{
		if (temp->next != NULL)
		{
			temp = temp->next;
		}
		else 
		{
			temp = lwt_head;
		}
	}
#ifdef __DEBUG

	printf("temp ip = %d, sp = %d\n",temp->ip,temp->sp);
	printf("head ip = %d, sp = %d\n",lwt_head->ip,lwt_head->sp);
	
	printf("current id = %d,ip = %d, sp = %d\n",lwt_curr->id,lwt_curr->ip,lwt_curr->sp);
#endif
	if (temp != lwt_curr)
	{
		//printf("/////Yielding to %d\n", temp->id);
		lwt_des = temp;

		struct lwt_context *curr,*next;
		curr = (struct lwt_context *) lwt_curr;
		next = (struct lwt_context *) lwt_des;
		//memcpy(&curr,lwt_curr,sizeof(struct lwt_context));
    	//memcpy(&next,lwt_des,sizeof(struct lwt_context));
#ifdef __DEBUG
		DEBUG();
#endif
		if(lwt_des != lwt_head)
			__lwt_dispatch(curr, next);
		else
		{
			struct lwt_context current;
			memcpy(&current,lwt_curr,sizeof(struct lwt_context));
			lwt_curr = lwt_des;
			__lwt_dispatch(&current,next);
		}
#ifdef _DEBUG
		DEBUG();
#endif
	}
#ifdef __DEBUG
	DEBUG();
#endif
	return;	
}

void enqueue(lwt_t lwt)
{
	lwt->prev = lwt_tail;
	lwt_tail->next = lwt;
	lwt_tail = lwt;
}
