#include<stdio.h>
#include<stdlib.h>
#include"lwt_dispatch.h"
#include"lwt.h"


void __lwt_start();
void __lwt_schedule(void);
//void __lwt_dispatch(lwt_t curr, lwt_t next);	
void enqueue(lwt_t lwt);
void* dequeue(lwt_t lwt);
void *__lwt_stack_get(void);
void __lwt_stack_return(void *stk);
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

/*
 * head of the queue of stack
 */
stack_t stack_head = NULL;

/*
 * tail node of the queue of stack
 */
stack_t stack_tail = NULL;

/*
 * number of available stack in the queue
 */
ulong stack_counter = 0;

lwt_t lwt_create(lwt_fn_t fn, void *data)
{

	lwt_t lwt_new;
	/*
	 * if current thread is NULL, make the new thread be the head
	 */
	if (lwt_head == LWT_NULL)
	{
		lwt_head = (lwt_t)malloc(sizeof(struct _lwt_t));
		lwt_head->ip = (ulong)0;
		lwt_head->sp = (ulong)NULL;

		stack_head = malloc(sizeof(struct _stack_t));
		stack_head->bsp =  (ulong) NULL ;	//be used to free the stack
		stack_head->flag = 1;
		stack_tail = stack_head;
		stack_head->next = NULL;

		lwt_head->stack = stack_head;

		lwt_head->id = lwt_counter++;
		lwt_curr = lwt_head;
		lwt_curr->status = LWT_ACTIVE;
		runable_counter++;
		lwt_tail = lwt_head;
	}
	
	//malloc the new thread and stack







	lwt_new = (lwt_t)malloc(sizeof(struct _lwt_t));
	lwt_new->id = lwt_counter++;
	lwt_new->ip = (ulong) (&__lwt_start);
	lwt_new->stack = __lwt_stack_get();
	lwt_new->sp = lwt_new->stack->bsp + STACK_SIZE - 4; 		
	lwt_new->status = LWT_ACTIVE;
	lwt_new->joiner = LWT_NULL; 
	lwt_new->fn = fn;
	lwt_new->data = data;
	runable_counter++;
	//lwt_des = lwt_new;
	
	enqueue(lwt_new);	
#ifdef __DEBUG
	lwt_t tmp = lwt_head;

	while(tmp->next != NULL)
	{
		printf("[create]temp id = %d, status = %d, ip = %d, sp = %d\n",tmp->id,tmp->status,tmp->ip,tmp->sp);
		tmp = tmp->next;
	}
#endif
	return lwt_new;
}

void *lwt_join(lwt_t lwt)
{
#ifdef __DEBUG
//	DEBUG();
#endif

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

	if(lwt->status != LWT_DEAD)
	{
#ifdef _DEBUG
		printf("lwt_curr id = %d\n",lwt_curr->id);
		printf("lwt_curr status = %d\n",lwt_curr->status);
		printf("lwt id = %d\n",lwt->id);
		printf("lwt status = %d\n",lwt->status);
#endif	
		lwt_curr->status = LWT_BLOCKED;
		blocked_counter++;
		runable_counter--;
		lwt_yield(lwt);
	//	if(lwt_curr->status = LWT_DEAD)
	//		lwt_curr = lwt_head;
	}

	temp_data = dequeue(lwt);
	died_counter--;
/*	//free stack
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
	free(lwt);*/
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
	if(lwt_curr->target != LWT_NULL && lwt_curr->target->status == LWT_ACTIVE)
		lwt_yield(lwt_curr->target);
	else
		lwt_yield(lwt_head);
//	lwt_yield(LWT_NULL);
}

int lwt_yield(lwt_t lwt)
{
#ifdef __DEBUG
	DEBUG();	

#endif
	if(lwt == NULL)
	{
#ifdef __DEBUG
		DEBUG();
#endif
		__lwt_schedule();
	
	}
	else if (lwt->status == LWT_ACTIVE)
	{
		lwt_des = lwt;
		
#ifdef __DEBUG
//	DEBUG();
//	printf("lwt id = %d,status = %d, ip = %d, sp = %d\n",lwt->id,lwt->status,lwt->ip,lwt->sp);
//	printf("cur id = %d, ip = %d, sp = %d\n",lwt_curr->id,lwt_curr->ip,lwt_curr->sp);
//	printf("des id = %d, ip = %d, sp = %d\n",lwt_des->id,lwt_des->ip,lwt_des->sp);
#endif
		lwt_t tmp_curr = lwt_curr;
		lwt_curr = lwt_des;
		__lwt_dispatch((struct lwt_context *)tmp_curr,(struct lwt_context *)lwt_des);
	}
#ifdef __DEBUG
	else 
	{
		printf("status = %d\n",lwt->status);
		printf("head status = %d\n",lwt_head->status);
	}
#endif
	return -1;
}

lwt_t lwt_current(void)
{
#ifdef __DEBUG
//	DEBUG();
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
//	DEBUG();
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
//	DEBUG();
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
	printf("data = %d\n",lwt_curr->data);
#endif
//	lwt_curr = lwt_des;
		
	void *return_val = lwt_curr->fn(lwt_curr->data);
#ifdef __DEBUG
	DEBUG();
#endif
	lwt_die(return_val);

#ifdef __DEBUG
//	DEBUG();
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
	
#ifdef __DEBUG
DEBUG();
#endif
	
	//get next ACTIVE thread
	if (lwt_head->next != LWT_NULL) 
	{
		temp = lwt_head->next;
	} 
	else 
	{
#ifdef __DEBUG
	//	printf("thread4 id = %d, ip = %d, sp = %d\n",lwt_curr->prev->id,lwt_curr->prev->ip,lwt_curr->prev->sp);
	DEBUG();
#endif

		temp = lwt_head;
	}
#ifdef __DEBUG
DEBUG();

	lwt_t tmp = lwt_head;

	while(tmp->next != NULL)
	{
		printf("temp id = %d, status = %d, ip = %d, sp = %d\n",tmp->id,tmp->status,tmp->ip,tmp->sp);
		tmp = tmp->next;
	}
#endif
/*	while (temp->status != LWT_ACTIVE)
	{
		if (temp->next != NULL)
		{
			temp = temp->next;
		}
		else 
		{
			temp = lwt_head;
		}
		//printf("temp id = %d, status = %d, ip = %d, sp = %d\n",temp->id,temp->status,temp->ip,temp->sp);
	}*/
	while(temp != lwt_head)// && (temp->status != LWT_ACTIVE ||  temp == lwt_curr))
	{
		if(temp != lwt_curr && temp->status == LWT_ACTIVE)
			break;
		if(temp->next != NULL)
		{
			temp = temp->next;
		}
		else
		{
			temp = lwt_head;
		}
//		printf("temp id = %d, status = %d, next = %d\n",temp->id,temp->status,temp->next->id);
	}
	if(temp->status != LWT_ACTIVE)
	{
		temp = lwt_curr;
	}
#ifdef __DEBUG

	printf("des id = %d, status = %d, ip = %d, sp = %d\n",temp->id,temp->status,temp->ip,temp->sp);
	printf("head id = %d, status = %d, ip = %d, sp = %d\n",lwt_head->id,lwt_head->status,lwt_head->ip,lwt_head->sp);
	
	printf("curr id = %d, status = %d, ip = %d, sp = %d\n",lwt_curr->id,lwt_curr->status,lwt_curr->ip,lwt_curr->sp);
#endif

	if (temp != lwt_curr)
	{
		lwt_des = temp;

		lwt_t tmp_curr = lwt_curr;
		lwt_curr = lwt_des;
		__lwt_dispatch((struct lwt_context *)tmp_curr, (struct lwt_context *)lwt_des);
#ifdef _DEBUG
	DEBUG();
#endif
	}
#ifdef __DEBUG
	DEBUG();
#endif
	return;	
}

/*void __lwt_dispatch(lwt_t curr, lwt_t next)	
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
			: "cc", "memory"
																							);
			//for switching back to old thread, nested?
			curr = next;
			return;	
}*/
void enqueue(lwt_t lwt)
{
	lwt->next = NULL;
	lwt->prev = lwt_tail;
	lwt_tail->next = lwt;
	lwt_tail = lwt;
}

void* dequeue(lwt_t lwt)
{
	void * temp_data;
	__lwt_stack_return(lwt->stack);
//	free((void *)lwt->bsp);
	lwt->prev->next = lwt->next;
	if(lwt->next != LWT_NULL)
		lwt->next->prev = lwt->prev;
	else
		lwt_tail = lwt->prev;
	temp_data = lwt->return_val;
	free(lwt);
	return temp_data;
}

void *__lwt_stack_get()
{

	if(stack_counter == 0)
	{
		stack_tail->next = malloc(sizeof(struct _stack_t));
		stack_tail = stack_tail->next;
		stack_tail->next = NULL;
		stack_tail->bsp =  (ulong) (malloc(STACK_SIZE)) ;	//be used to free the stack
		stack_tail->flag = 1;
		return stack_tail;
	}
	else
	{
		stack_t tmp = stack_head;		
		while(tmp->flag != 0)
		{
			tmp = tmp->next;
		}
		tmp->flag = 1;
		stack_counter--;
		return tmp;
	}	
}
void __lwt_stack_return(void *stk)
{
	stack_t tmp = (stack_t) stk;
	stack_counter++;
	tmp->flag = 0;
}
