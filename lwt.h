#include"ps_list.h"
#ifndef _LWT_H
#define _LWT_H

//#define __DEBUG

#define DEBUG(format,...) printf("[DEBUG INFO]: FILE %s, LINE %d"format"\n",__FILE__,__LINE__, ##__VA_ARGS__)
#ifdef __DEBUG

#endif
#ifndef STACK_SIZE
#define STACK_SIZE 4096
#endif

#define LWT_NULL NULL

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef void *(*lwt_fn_t) (void *);
typedef enum {
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

typedef struct _lwt_t
{
	ulong ip;
	ulong sp;
//	stack_t stack;
	ulong bsp;
	uint id;
	lwt_status_t status;	
	lwt_fn_t fn;
	void *data;
	void *return_val;
	uchar ifrecycled;	//to identify if this lwt is now in the pool
	int valid_val;		//to identify if this lwt is a vaild sender to a specific channel
	struct _lwt_t *joiner;
	struct _lwt_t *target;
	struct ps_list list;
}*lwt_t;

typedef struct _global_counter_t
{
	/*
 	* auto-increment counter for thread
 	*/
	uint lwt_count;	
	
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
 	* counter for available thread
	*/
	uint avail_counter;

	/*
	 * counter for number of channels that are active
	 */
	uint nchan_counter;

	/*
	 * (number of threads blocked sending on channels),
	 */
	uint nsnding_counter;

	/*
	 * (number of threads blocked receiving on channels),
	 */
	uint nrcving_counter;
}global_counter_t;

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

global_counter_t gcounter;

lwt_t lwt_create(lwt_fn_t fn, void *data);
void *lwt_join(lwt_t thread);
void lwt_die(void *data);
int lwt_yield(lwt_t lwt);
lwt_t lwt_current(void);
int lwt_id(lwt_t lwt);
int lwt_info(lwt_info_t t);
void __lwt_start();
void *__lwt_stack_get(void);
#endif
