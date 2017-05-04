#ifndef KALLOC_H
#define KALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <cos_kernel_api.h>

/* Modify to meet your requirement, if necessary */
#ifndef PAGENUM
	#define PAGENUM (4 * 1024)
#endif


#ifndef PAGESIZE
	#define PAGESIZE (PAGENUM * 4096)
#endif



void	kinit(void);					/* init the pool, necessary*/
char*	kalloc(int n);
void	kfree(char *v, int len);

#endif	/* KALLOC_H */
