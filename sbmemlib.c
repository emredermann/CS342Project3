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


// Defined the length randomly
#define len 256

//https://www.geeksforgeeks.org/buddy-memory-allocation-program-set-1-allocation/

//Check Those links for shared memory segment


// Shared memory object
int fd;
char *ptr;
sem_t semvar;

int sbmem_init(int segsize){

    // Removes the previous (if exist) shared memory.
    if (shm_unlink( "/sharedMem" ))
    {
        printf("Shared memory unlinked.");
    }

    fd = shm_open("/sharedMem",O_RDWR | O_CREAT, 0777 );

  if(fd == -1){
      printf("Error Open shared memory open \n");
      return -1;
  }  
    /* Set the memory object's size  */
    //The size of part ??
    if( ftruncate( fd, sizeof( segsize ) ) == -1 ) {
        printf("ftruncate error \n");
        return -1;
    }
    return 0;
}


// Have doubts about 
// The created semaphore(s) will be removed as well. 
bool sbmem_remove (){
    if(shm_unlink("/sharedMem"))
        return true;
    return false;
}


int sbmem_open(){
    //library mapped the shared segment.
    ptr = mmap( 0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    if(ptr == MAP_FAILED){
        printf( "Mmap failed: \n");
        return -1;
    }
    return 0;
}


void *sbmem_alloc (int reqsize){

    char *ptr= malloc( nextPower(reqsize));
    if (ptr != NULL)
        return ptr;
    printf("Memory could not allocated");
    return NULL;
}

//To find the necessary size of the allocation.
int nextPower(int num){
    int i = 1;
    while(1)
    {
        if( (int) pow(2,i) > num){
            return (int) pow(2,i);
        }
        i++;
    }
}


void sbmem_free (void *ptr){
    free(ptr);
}

//OPTIONAL FOR THE PROJECT
int sbmem_close (){
    if(close( fd ) && munmap( ptr, len ))
        return 1;
    return -1;
}