#include"ps_list.h"
#ifndef _LWT_H
#define _LWT_H

//#define __DEBUG

#define DEBUG(format,...) printf("[DEBUG INFO]: FILE %s, LINE %d"format"\n",__FILE__,__LINE__, ##__VA_ARGS__)
#ifdef __DEBUG

#endif
#ifndef STACK_SIZE
#define STACK_SIZE 3074 
#endif

#define RING_SIZE 1024

#define LWT_NULL NULL

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef void *(*lwt_fn_t) (void *);
typedef enum 
{
	LWT_INFO_NTHD_RUNNABLE,
	LWT_INFO_NTHD_BLOCKED,
	LWT_INFO_NTHD_ZOMBIES,
	LWT_INFO_NCHAN,
	LWT_INFO_NSNDING,
	LWT_INFO_NRCVING,
} lwt_info_t;

typedef enum
{
	LWT_ACTIVE,
	LWT_BLOCKED,
	LWT_DEAD,
	LWT_WAITING,
}lwt_status_t;

typedef enum
{
	LWT_JOIN,
	LWT_NOJOIN,
}lwt_flags_t;

typedef struct _lwt_t
{
	ulong ip;
	ulong sp;
	uint id;
	lwt_status_t status;
	lwt_flags_t lwt_nojoin;	
	lwt_fn_t fn;
//	struct sl_thd* kthd;
	uint tid;
	void* data;
	void *return_val;
	struct _lwt_t *joiner;
	struct _lwt_t *target;
	struct ps_list list;
}*lwt_t;

typedef struct _global_counter_t
{
	/*auto-increment counter for thread*/
	uint lwt_count;		
	/*counter for runable thread*/
	uint runable_counter;
	/*counter for blocked thread*/
	uint blocked_counter;
	/*counter for died thread*/
	uint died_counter;
	/*counter for number of channels that are active*/
	uint nchan_counter;
	/*(number of threads blocked sending on channels)*/
	uint nsnding_counter;
	/*(number of threads blocked receiving on channels)*/
	uint nrcving_counter;
	uint nchan_id;
}global_counter_t;

typedef struct ring_buffer 
{
	int size;
	int start;
	int end;
	void** data;  
} ring_buffer;

typedef struct cgroup
{
	int id;
	int n_chan;          //number of channels in the group
	//the channel that ready to receive, point to the head of the event queue   
	struct lwt_channel *events;
//  	struct ps_list list;	
} cgroup, *lwt_cgrp_t;

typedef struct clist_head
{
	void* data;
	lwt_t thd;
	struct ps_list list;
} clist_head, *clist_t;

typedef struct lwt_channel 
{
	int id;     /* channel's id*/
	/* sender’s data */
	int snd_cnt; 				/* number of sending threads */
	clist_t snd_thds;
	/* receiver’s data */
	int rcv_blocked;
	lwt_t rcv_thd;	 			/* the receiver */
	void *mark;
	/* chan's data */
	int iscgrp;            //check if it added to a group
	lwt_cgrp_t cgrp;		// belongs to which group
//	int size;				// size of the buffer
	ring_buffer data_buffer;
	struct ps_list list;
} lwt_channel, *lwt_chan_t;
/*
typedef struct __kthd_parm_t
{
	struct sl_thd* kthd;
	lwt_t lwt;
	lwt_fn_t fn;
	lwt_chan_t c;
}*kthd_parm_t;*/
typedef void *(*lwt_chan_fn_t)(lwt_chan_t);

/* head of the queue of  thread*/
lwt_t lwt_head;


global_counter_t gcounter;

lwt_t lwt_create(lwt_fn_t fn, void *data, lwt_flags_t flags);
void *lwt_join(lwt_t thread);
void lwt_die(void *data);
int lwt_yield(lwt_t lwt);
lwt_t lwt_current(void);
int lwt_id(lwt_t lwt);
int lwt_info(lwt_info_t t);
void __lwt_start();
void *__lwt_stack_get(void);
lwt_chan_t lwt_chan(int sz);
void lwt_chan_deref(lwt_chan_t c);
int lwt_snd(lwt_chan_t c,void *data);
void *lwt_rcv(lwt_chan_t c);
int lwt_snd_chan(lwt_chan_t c,lwt_chan_t sending);
lwt_chan_t lwt_rcv_chan(lwt_chan_t c);
lwt_t lwt_create_chan(lwt_chan_fn_t fn, lwt_chan_t c, lwt_flags_t flag);
lwt_cgrp_t lwt_cgrp(void);
int lwt_cgrp_free(lwt_cgrp_t cgrp);
int lwt_cgrp_add(lwt_cgrp_t cgrp, lwt_chan_t c);
int lwt_cgrp_rem(lwt_cgrp_t cgrp, lwt_chan_t c);
lwt_chan_t lwt_cgrp_wait(lwt_cgrp_t cgrp);
void lwt_chan_mark_set(lwt_chan_t c, void *data);
void *lwt_chan_mark_get(lwt_chan_t);
int lwt_kth_create(lwt_fn_t fn, lwt_chan_t c);
int lwt_snd_thd(lwt_chan_t c,lwt_t sending);
void *lwt_rcv_thd(lwt_chan_t c);
#endif
