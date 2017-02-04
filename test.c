#include"stdio.h"
#include"pthread.h"
#include"lwt_dispatch.h"
struct lwt_context a,b;
void *test()
{
	int i=0;
	while(i<5)
	{
		printf("d = %d\n",i);
		i++;
	}
}
void *test1()
{
	int i=0;
	while(i<5)
	{
		printf("f = %d\n",i);
		i++;
	}
	
}

void ttt()
{
	printf("test\n");
	printf("ip = %d\n",b.ip);
	printf("sp = %d\n",b.sp);
	__lwt_dispatch(&a,&b);
	printf("uuuuuu\n");
}
void main()
{
	pthread_t pd1,pd2;
	pthread_create(&pd1,NULL, test, NULL);
	pthread_create(&pd2,NULL, test1, NULL);


	printf("a = 1\n");

pthread_join(pd1,NULL);
	a.ip =  &ttt;
	a.sp =  malloc(4096) + 4096 -4;
	b.ip = 0;
	b.sp = NULL;
	printf("sp = %d\n",b.sp);
	__lwt_dispatch(&b,&a);
	printf("ip = %d\n",b.ip);

	printf("sp = %d\n",b.sp);
	printf("iii\n");
}
