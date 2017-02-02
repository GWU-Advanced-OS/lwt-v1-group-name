#ifndef _LWT_H
#define _LWT_H

#define STACK_SIZE 4096

#define LWT_NULL NULL

#define LWT_INFO_NTHD_RUNNABLE 0x1;
#define LWT_INFO_NTHD_ZOMBIES 0x2;
#define LWT_INFO_NTHD_BLOCKED 0x3;

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef void *(*lwt_fn_t) (void *);
typedef int lwt_info_t
typedef enum
{
	_ACTIVE,
	_DEAD,
	_BLOCKED,
	_WAITING,
}_TCB_STATE;

typedef struct lwt_tcb
{
	int id;
	ulong ip;
	ulong sp;
}lwt_tcb, *lwt_t;

/*
 * auto-increment counter for thread
 */
int lwt_counter;	

lwt_info_t LWT_INFO_NTHD_RUNNABLE;
lwt_info_t LWT_INFO_NTHD_ZOMBIES;
lwt_info_t LWT_INFO_NTHD_BLOCKED;

lwt_t lwt_create(lwt_fn_t fn, void *data);
void *lwt_join(lwt_t thread);
void lwt_die(void *data);
int lwt_yield(lwt_t destination);
lwt_t lwt_current(void);
int lwt_id(lwt_t tcb);
int lwt_info(lwt_info_t t);
#endif
