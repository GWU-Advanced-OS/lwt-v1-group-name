/*#include<stdio.h>*/
#include<stdlib.h>
#include"lwt_dispatch.h"
#include"lwt.h"
void __lwt_start();
void enqueue(lwt_t lwt);


lwt_t lwt_create(lwt_fn_t fn, void *data)
{
	/*
	 * if current thread is NULL, create one
	 */
	if (lwt_head == LWT_NULL)
	{
		lwt_head = (lwt_t)malloc(sizeof(struct _lwt_t));
		lwt_current = lwt_head;
		lwt_current->status = LWT_RUNNING;
		counter_runable++;
		lwt_tail = lwt_current;
	}

	//malloc the new thread and stack
	lwt_des = (lwt_t)malloc(sizeof(struct _lwt_t));
	lwt_counter++;
	lwt_des->id = lwt_counter;
	lwt_des->ip = &__lwt_start;
	lwt_des->sp = malloc(STACK_SIZE) + STACK_SIZE - 4; 		//for free stack
	lwt_des->state = LWT_READY;
	lwt_des->parent_id = lwt_current->id;

	lwt_des->joiner = LWT_NULL; 
	lwt_des->fn = fn;
	lwt_des->data = data;
	runable_counter++;

	enqueue(lwt_des);	
	return lwt_des;
}

void *lwt_join(lwt_t thread)
{

}

void lwt_die(void *data)
{
	//seriously, prevent when joining(A), the finish of B changes the state of main thread
	//if (current_tcb == current_tcb->target)
	if (lwt_current->target != NULL)
	{
		lwt_current->target->status = LWT_READY;
	}
	//mark its state as DEAD, free the stack when......?
	lwt_current->status = LWT_DEAD;

	let_current->return_val = (void *)data;

	lwt_yield(LWT_NULL);
}

int lwt_yield(lwt_t destination)
{

}

lwt_t lwt_current(void)
{

}

int lwt_id(lwt_t lwt)
{
	return lwt->id;
}

int lwt_info(lwt_info_t t)
{

}

void __lwt_start()
{
	lwt_current = lwt_des;
	void *return_val = lwt_current->fn(lwt_current->data);
	lwt_die(return_val);
}

void enqueue(lwt_t lwt)
{
	lwt->prev = lwt_tail;
	lwt_tail->next = lwt;
	lwt_tail = lwt;
}
