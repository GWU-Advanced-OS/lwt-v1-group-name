#ifndef _LWT_H
#define _LWT_H

#define __DEBUG

#ifdef __DEBUG
#define DEBUG(format,...) printf("[DEBUG INFO]: LINE %d"format"\n",__LINE__, ##__VA_ARGS__)
#endif

#define STACK_SIZE 4096

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
	LWT_DEAD
}lwt_status_t;

typedef struct _lwt_t
{
	ulong ip;
	ulong sp;
	uint id;
	lwt_status_t status;	
	lwt_fn_t fn;
	void *data;
	void *return_val;
	struct _lwt_t *joiner;
	struct _lwt_t *target;
	struct _lwt_t *next;
	struct _lwt_t *prev;
}*lwt_t;



lwt_t lwt_create(lwt_fn_t fn, void *data);
void *lwt_join(lwt_t thread);
void lwt_die(void *data);
int lwt_yield(lwt_t lwt);
lwt_t lwt_current(void);
int lwt_id(lwt_t lwt);
int lwt_info(lwt_info_t t);
#endif
