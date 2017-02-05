#include"stdio.h"
#include"pthread.h"
#include"lwt_dispatch.h"
struct lwt_context a,b,c;
int flag=0;
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
	while(i<3)
	{
	
		i++;
	//	b.ip -= 100;
	//	b.sp -= 100;
	printf("des ip = %d, sp = %d\n",b.ip,b.sp);
	printf("head ip = %d, sp = %d\n",b.ip,b.sp);
	printf("cur ip = %d, sp = %d\n\n",c.ip,c.sp);
	__lwt_dispatch(&c,&b);
	}
	
}

void sss()
{

	printf("des ip = %d, sp = %d\n",c.ip,c.sp);
	printf("head ip = %d, sp = %d\n",b.ip,b.sp);
	printf("cur ip = %d, sp = %d\n\n",a.ip,a.sp);
	__lwt_dispatch(&a,&c);
	printf("des ip = %d, sp = %d\n",c.ip,c.sp);
	printf("head ip = %d, sp = %d\n",b.ip,b.sp);
	printf("cur ip = %d, sp = %d\n\n",a.ip,a.sp);
	__lwt_dispatch(&a,&c);
	printf("des ip = %d, sp = %d\n",c.ip,c.sp);
	printf("head ip = %d, sp = %d\n",b.ip,b.sp);
	printf("cur ip = %d, sp = %d\n\n",a.ip,a.sp);
	__lwt_dispatch(&a,&c);

	printf("des ip = %d, sp = %d\n",c.ip,c.sp);
	printf("head ip = %d, sp = %d\n",b.ip,b.sp);
	printf("cur ip = %d, sp = %d\n",a.ip,a.sp);

}

void ttt()
{
	printf("a ip = %d, sp = %d\n",a.ip,a.sp);
	printf("b ip = %d, sp = %d\n",b.ip,b.sp);
	printf("c ip = %d, sp = %d\n",c.ip,c.sp);
	if(flag == 0)
	{	
		flag = 1;
		printf("first to execute ttt\n");
		__lwt_dispatch(&a,&c);
		
		printf("third to execute ttt\n");
		__lwt_dispatch(&a,&b);
	}	
	else
	{
		printf("second to execute ttt\n");
		__lwt_dispatch(&c,&a);
	}
}
void main()
{
//	pthread_t pd1,pd2;
//	pthread_create(&pd1,NULL, test, NULL);
//	pthread_create(&pd2,NULL, test1, NULL);


//	printf("a = 1\n");

//	pthread_join(pd1,NULL);
	a.ip =  &sss;
	a.sp =  malloc(4096) + 4096 -4;
	b.ip = 0;
	b.sp = NULL;
	c.ip = &test1;
	c.sp = malloc(4096) + 4096 - 4;	
//	printf("a ip = %d, sp = %d\n",a.ip,a.sp);
//	printf("b ip = %d, sp = %d\n",b.ip,b.sp);
//	printf("c ip = %d, sp = %d\n",c.ip,c.sp);
	printf("des ip = %d, sp = %d\n",a.ip,a.sp);
	printf("head ip = %d, sp = %d\n",b.ip,b.sp);
	printf("cur ip = %d, sp = %d\n\n",b.ip,b.sp);
	__lwt_dispatch(&b,&a);
	printf("des ip = %d, sp = %d\n",a.ip,a.sp);
	printf("head ip = %d, sp = %d\n",b.ip,b.sp);
	printf("cur ip = %d, sp = %d\n\n",b.ip,b.sp);
	__lwt_dispatch(&b,&a);
	printf("des ip = %d, sp = %d\n",a.ip,a.sp);
	printf("head ip = %d, sp = %d\n",b.ip,b.sp);
	printf("cur ip = %d, sp = %d\n\n",b.ip,b.sp);
	__lwt_dispatch(&b,&a);
	
//	printf("a ip = %d, sp = %d\n",a.ip,a.sp);
//	printf("b ip = %d, sp = %d\n",b.ip,b.sp);
//	printf("c ip = %d, sp = %d\n",c.ip,c.sp);

	printf("iii\n");
}
