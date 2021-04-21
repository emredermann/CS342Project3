#include "sbmem.h"
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>

struct block{
    int address;
    int limit;
    struct block * next;
};

block * p_map;
bool activeProcess;
int virtualAddress;

int sbmem_init (int segsize){
    int fd;
    if (shm_unlink( "/sharedMem" ))
    {
        printf("Shared memory unlinked.");
        return 0;
    }
    fd = shm_open("/sharedMem",O_RDWR | O_CREAT, 0777 );

    if(fd == -1){
      printf("Error Open shared memory open \n");
     
      return -1;
   }  

   if(segsize > 262144 || segsize < 32768){
       printf("Error");
       exit(-1);
   }
    if( ftruncate( fd,  segsize) == -1 ) {
        printf("ftruncate error \n");
        return -1;
    }
    //Initializes the shared memory mapps it to the p_map
    // Size of the mapped segment.
    p_map = (block *) mmap( virtualAddress, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    virtualAddress += sizeof(block) * 9;
    //linkedlistInit(p_map);
    
    if(p_map == MAP_FAILED){
        printf( "Mmap failed: \n");
        return -1;
    }
    return 0;
    
}
