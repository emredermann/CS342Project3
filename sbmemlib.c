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


//mmap implementation
//https://yandex.com/turbo/devsday.ru/s/blog/details/21607


// Shared memory object

struct entry{
    pid_t pid;
    long baseAddress;
    long limit;
};
int current_counter;
int fd;
struct entry **ptr;
sem_t semvar;



int sbmem_init(int segsize){
    current_counter = 0;
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

    //Initializes the shared memory
      ptr = mmap( 0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    if(ptr == MAP_FAILED){
        printf( "Mmap failed: \n");
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
  
    return 0;
}


void *sbmem_alloc (int reqsize){
    
    //instead of malloc
    // ?????? not sure how much correct for allocation of the space.
    struct entry * tmp = malloc( nextPower(reqsize));
   
    if (tmp != NULL){
         ptr[current_counter++] = tmp;
         return tmp;
        }
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
