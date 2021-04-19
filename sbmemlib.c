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
 #define MINIMUN_SEGMENT_SIZE 512
//https://www.geeksforgeeks.org/buddy-memory-allocation-program-set-1-allocation/
//https://www.tutorialspoint.com/inter_process_communication/inter_process_communication_shared_memory.htm

// Important One
//https://stackoverflow.com/questions/29238015/creating-shared-memory-segments

//mmap implementation
//https://yandex.com/turbo/devsday.ru/s/blog/details/21607


// Shared memory object
// The attributes or the structure must be checked. 
typedef struct {
    pid_t pid;
    // Current location of the block 
    int baseAddress;
    // Limit of the block
    int limit;
    block * next;
}block;


block * p_map;
int current_counter;
int fd;
 

int counter;

int sbmem_init(int segsize){
    
    // Removes the previous (if exist) shared memory.
  
    if (shm_unlink( "/sharedMem" ))
    {
        printf("Shared memory unlinked.");

        return 0;
    }

    // Creates shared memory
    fd = shm_open("/sharedMem",O_RDWR | O_CREAT, 0777 );
    
    
    if(fd == -1){
      printf("Error Open shared memory open \n");
     
      return -1;
   }  
    /* Set the memory object's size  */
    //The size of part ??
    if( ftruncate( fd, sizeof( nextPower(segsize) ) ) == -1 ) {
        printf("ftruncate error \n");
        return -1;
    }

    //Initializes the shared memory mapps it to the p_map
    // Size of the mapped segment.
    p_map = (block *) mmap( 0, sizeof(block) * nextPower(segsize), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    p_map->next == NULL;

    if(p_map == MAP_FAILED){
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

 
int virtualAddress = p_map + 4;

p_map = (block *) mmap( virtualAddress, sizeof(block) * nextPower(MINIMUN_SEGMENT_SIZE), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );

    
    return 0;
}


void *sbmem_alloc (int reqsize){
 
    
    if (p_map != NULL){

         return p_map;
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
 