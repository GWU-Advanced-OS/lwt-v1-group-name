#include"lwt_dispatch.h"

void enqueue(lwt_t lwt)
{
	lwt->pre = lwt_tail;
	lwt_tail->newt = lwt;
	lwt_tail = lwt;
}

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
	lwt_des->ip = NULL;
	lwt_des->sp = malloc(STACK_SIZE) + STACK_SIZE - 4; 		//for free stack
	lwt_des->state = LWT_READY;
	lwt_des->parent_id = lwt_current->id;

	lwt_des->fn = fn;
	lwt_des->data = data;
	runable_counter++;

	enqueue(lwt_des);	
	return lwt_des;
}

void *lwt_join(lwt_t thread)
{

}

void lwt_die(void * data)
{

}

int lwt_yield(lwt_t destination)
{

}

lwt_t lwt_current(void)
{

}

int lwt_id(lwt_t tcb)
{
	return tcb->id;
}

int lwt_info(lwt_info_t t)
{

}
