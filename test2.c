#include"stdio.h"
void main()
{
	void *a = 2;
	a = (char *)a;
	printf("%d\n",a);
}
