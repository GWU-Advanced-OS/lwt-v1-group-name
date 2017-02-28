#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include "channel.h"

int nchan_num = 0;

void 
enqueue_snd(lwt_chan_t c, clist_t new)
{
	if(c->snd_thds == NULL) 
	{
		c->snd_thds = new;
		c->tail_snd = new;
		return;
	}
   	else 
	{
		c->tail_snd->next = new;
		c->tail_snd = new;
		return;
	}
}

void 
dequeue_snd(lwt_chan_t c)
{
	if (c->snd_thds == NULL) 
	{
		return;
	}
	if (c->snd_thds->next == NULL) 
	{
		free(c->snd_thds);
		c->snd_thds = NULL;
		c->tail_snd = NULL;
		return;
	} 
	else 
	{
		clist_t temp = c->snd_thds;
		c->snd_thds = c->snd_thds->next;
		free(temp);
		return;
	}
}

void 
enqueue_snd_list(lwt_chan_t c, chan_list_t new)
{
	if(c->snd_list_head == NULL) 
	{
		c->snd_list_head = new;
		c->snd_list_tail = new;
		return;
	}
   	else 
	{
		c->snd_list_tail->next = new;
		c->snd_list_tail = new;
		return;
	}
}

int 
__valid_sender(lwt_chan_t c, lwt_t lwt)
{
	if(c->id == lwt->valid_val)
	{
		return 1;
	}
	chan_list_t temp = c->snd_list_head;	
	while(temp != NULL)
	{
		if(temp->thd == lwt)
		{
			return 1;
		}
		temp = temp->next;
	}
	return 0;
//	return 1;

}
/* 
 * Currently assume that sz is always 0. 
 * This function uses malloc to allocate 
 * the memory for the channel. 
 */

lwt_chan_t 
lwt_chan(int sz)
{

	lwt_chan_t new = (lwt_chan_t)malloc(sizeof(lwt_channel));
	new->snd_cnt = sz;
	new->id = nchan_num++;
	gcounter.nchan_counter++;
	new->snd_thds = NULL;
	new->rcv_thd = lwt_curr;
//	new->rcv_thd->blocked = 0;
	return new;
}

/*
 * Deallocate the channel if no threads still 
 * have references to the channel.
 */

void 
lwt_chan_deref(lwt_chan_t c)
{
	if (c->snd_thds == NULL && c->rcv_thd->status != LWT_WAITING) 
	{
		gcounter.nchan_counter--;
		free(c); 	//will also set "rcv_thd" as NULL
		return;
	}
	printf("Cannot deallocate the channel.\n");
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

	if(!__valid_sender(c,lwt_curr))
	{
		return -2;
	}

	//if there is node that is new added without filling the data,fill it 
	clist_t new_clist = (clist_t)malloc(sizeof(clist_head));
	new_clist->thd = lwt_curr;
	new_clist->data = data;
	new_clist->next = NULL;
	c->snd_cnt++;
	enqueue_snd(c, new_clist);

	//block current thread
	gcounter.nsnding_counter++;

//	lwt_curr->blocked = 1;
	lwt_curr->status = LWT_WAITING;
	
	while (c->rcv_thd->status == LWT_ACTIVE)//blocked == 0) 
	{

		lwt_yield(LWT_NULL);
	}

	c->rcv_thd->status = LWT_ACTIVE;//blocked = 0;
	gcounter.nsnding_counter--;

	//receiver start to recieve, unblock current sender
	return 0;
}

void 
*lwt_rcv(lwt_chan_t c)
{
	assert(c != NULL);

	if (c->rcv_thd != lwt_curr) 
	{
		return NULL;
	}
	
	c->rcv_thd->status = LWT_WAITING;//blocked = 1;

	gcounter.nrcving_counter++;

	//if snd_thds is null, block the reciever to wait for sender
	while (c->snd_thds == NULL) 
	{
		lwt_yield(LWT_NULL);
	}
		
	gcounter.nrcving_counter--;
	//somebody sent, so it can receive

	void *temp = c->snd_thds->data;
	c->snd_cnt--;
	dequeue_snd(c);
	c->rcv_thd->status = LWT_ACTIVE;//blocked = 0;

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

	chan_list_t list = malloc(sizeof(chan_list));	
	list->thd = c->rcv_thd;
	list->thd->valid_val = sending->id;
	enqueue_snd_list(c, list);
	sending->snd_cnt++;
	sending->rcv_thd = lwt_curr;

	

	if (c->rcv_thd == NULL)
   	{
		return -1;
	}

	if(!__valid_sender(c,lwt_curr))
	{
		return -2;
	}
	//if there is node that is new added without filling the data,fill it 
	clist_t new_clist = (clist_t)malloc(sizeof(clist_head));
	new_clist->thd = lwt_curr;
	new_clist->data = sending;
	new_clist->next = NULL;
	enqueue_snd(c, new_clist);

	
	//block current thread
	gcounter.nsnding_counter++;
	
	lwt_curr->status = LWT_WAITING;//blocked = 1;
	while (c->rcv_thd->status == LWT_ACTIVE)//blocked == 0) 
	{
		lwt_yield(LWT_NULL);
			
	}
	
	c->rcv_thd->status = LWT_ACTIVE;//blocked = 0;
	gcounter.nsnding_counter--;

	
	//receiver start to recieve, unblock current sender
	return 0;
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
	
	chan_list_t list = malloc(sizeof(chan_list));	
	list->thd = lwt;
	lwt->valid_val = c->id;
	enqueue_snd_list(c, list);
	c->snd_cnt++;

	return lwt;
}
