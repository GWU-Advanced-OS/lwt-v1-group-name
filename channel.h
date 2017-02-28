#include"lwt.h"
#include"ps_list.h"
#ifndef CHANNEL_H
#define CHANNEL_H
typedef struct chan_list
{
	lwt_t thd;
	struct chan_list *next;
}chan_list, *chan_list_t;

typedef struct clist_head
{
	int id;
	void* data;
//	int ifnew;
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
	chan_list_t snd_list_head;
	chan_list_t snd_list_tail;
	/* receiver’s data */
//	int rcv_blocked; 			/* if the receiver is blocked */
	lwt_t rcv_thd;	 			/* the receiver */
} lwt_channel, *lwt_chan_t;


typedef void *(*lwt_chan_fn_t)(lwt_chan_t);

#endif
