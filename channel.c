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
	new->rcv_blocked = 0;
	new->rcv_thd = lwt_curr;
	return new;
}

/*
 * Deallocate the channel if no threads still 
 * have references to the channel.
 */

void 
lwt_chan_deref(lwt_chan_t c)
{
	printf("num = %d\n",c->snd_cnt);
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
	
	//block current thread
//	lwt_curr->status = LWT_WAITING;
	gcounter.nsnding_counter++;
//	gcounter.runable_counter--;
	
	while (c->rcv_blocked == 1) 
	{
		lwt_yield(LWT_NULL);	
	}
	
	clist_t new_clist = (clist_t)malloc(sizeof(clist_head));
	new_clist->thd = lwt_curr;
	new_clist->data = data;
	new_clist->next = NULL;

	c->snd_cnt++;
	enqueue_snd(c, new_clist);
	
	c->rcv_blocked = 0;
//	c->rcv_thd->status = LWT_ACTIVE;
	gcounter.nsnding_counter--;
//	gcounter.runable_counter++;
	
	//receiver start to recieve, unblock current sender
	return 0;
}

void 
*lwt_rcv(lwt_chan_t c)
{
	assert(c != NULL);

	if (c->rcv_thd == NULL) 
	{
		c->rcv_thd = lwt_curr;
	}
	
//	c->rcv_blocked = 0;
//	c->rcv_thd->status = LWT_WAITING;
	gcounter.nrcving_counter++;
//	gcounter.runable_counter--;
	
	while (c->snd_thds == NULL) 
	{
		//printf("Rcv blocking! (because of NO sender in queue)\n");
		lwt_yield(LWT_NULL);
	}

	//somebody sent, so it can receive

	void *temp = c->snd_thds->data;
	c->snd_cnt--;
	dequeue_snd(c);

	c->rcv_blocked = 0;
//	c->rcv_thd->status = LWT_ACTIVE;
	gcounter.nrcving_counter--;
//	gcounter.runable_counter++;
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
	return lwt_snd(c,(void*) sending);
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
	
	return lwt_create((lwt_fn_t)fn,(void*)c);
}
