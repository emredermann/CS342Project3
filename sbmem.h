#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mman.h>


int sbmem_init (int segsize);

void sbmem_remove();
int sbmem_open();
void * sbmem_alloc (int reqsize);
void sbmem_free (void *ptr);
int sbmem_close ();
struct block*  combineBlocks(struct  block * ptr_1,struct  block * ptr_2);
struct  block* DivideBlock( int realsize);
void linkedlistInit(struct block * target);
int nextPower(int num);