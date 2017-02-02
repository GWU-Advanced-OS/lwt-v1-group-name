#include"lwt_dispatch.h"

lwt_t lwt_create(lwt_fn_t fn, void *data)
{

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
