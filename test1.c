#include"lwt.h"
#include"stdio.h"

void thread()
{
	int i =0;
	while(i++<5)
	{
		printf("thread address = %d, id = %d, round %d\n",lwt_current(),lwt_current()->id,i);
		lwt_yield(LWT_NULL);
	}
}

void yield()
{
	lwt_yield(LWT_NULL);
	lwt_yield(LWT_NULL);
	lwt_yield(LWT_NULL);

}

void thread1()
{
	lwt_yield(LWT_NULL);
	lwt_yield(LWT_NULL);
	lwt_yield(LWT_NULL);
	
}
void main()
{
	lwt_t t1,t2;
	t1 = lwt_create(yield,NULL);
	t2 = lwt_create(thread1,NULL);
//	lwt_join(t1);
	lwt_yield(LWT_NULL);
	printf("++++++++++++++++++++++++++++\n");
	lwt_yield(LWT_NULL);
	printf("++++++++++++++++++++++++++++\n");
	lwt_yield(LWT_NULL);
	printf("++++++++++++++++++++++++++++\n");
}
