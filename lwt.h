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
	LWT_INFO_NTHD_ZOMBIES
} lwt_info_t;

typedef enum
{
	LWT_ACTIVE,
	LWT_BLOCKED,
	LWT_DEAD,
}lwt_status_t;
/*
typedef struct _stack_t
{
	ulong bsp;	//the base of the stack pointer
	uchar flag;	//0 is free, 1 is busy;
	struct _stack_t *next;
}*stack_t;*/

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
}global_counter_t;

lwt_t lwt_create(lwt_fn_t fn, void *data);
void *lwt_join(lwt_t thread);
void lwt_die(void *data);
int lwt_yield(lwt_t lwt);
lwt_t lwt_current(void);
int lwt_id(lwt_t lwt);
int lwt_info(lwt_info_t t);
#endif
