#include"lwt.h"
#ifndef CHANNEL_H
#define CHANNEL_H

typedef struct clist_head
{
	int id;
	int ifnew;
	void* data;
	lwt_t thd;
	struct clist_head *next;

} clist_head, *clist_t;

typedef struct lwt_channel 
{
	int id;     /* channel's id*/
	/* sender’s data */
	//void *snd_data;
	int snd_cnt; 				/* number of sending threads */
	clist_t snd_thds;			/* list of those threads */
	clist_t tail_snd;
	/* receiver’s data */
	int rcv_blocked; 			/* if the receiver is blocked */
	lwt_t rcv_thd;	 			/* the receiver */
} lwt_channel, *lwt_chan_t;


typedef void *(*lwt_chan_fn_t)(lwt_chan_t);

#endif
