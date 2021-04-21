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
    int no_active_process;
    struct block * next;
};

struct block * page_addr;
struct block * p_map;
int virtualAddress;
int SEG_SIZE;
int fd;



int sbmem_init (int segsize){
    
    
    SEG_SIZE = segsize;
    if (shm_unlink( "/sharedMem" ))
    {
        printf("Shared memory unlinked.");
        return 0;
    }
    fd = shm_open("/sharedMem",O_RDWR | O_CREAT, 0666 );

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
    p_map = (struct block *) mmap( NULL, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    p_map->limit = SEG_SIZE;
    p_map->address = 0;
    p_map->no_active_process = 0;

  //  p_map = p_map->next;

    linkedlistInit(p_map);
    
    if(p_map == MAP_FAILED){
        printf( "Mmap failed: \n");
        return -1;
    }
    return 0;
    
}


void linkedlistInit(struct block * target){

    int i  = 7;
    struct block * new_block;
    new_block->limit=pow(2,i);
    new_block->address = 8;
    target->next = new_block;
    
    while(pow(2,i)< SEG_SIZE){
        struct block * tmp_block;
        tmp_block->limit = pow(2,i);
        tmp_block->address = new_block->address + new_block->limit;
        i++;
        new_block->next = tmp_block;
        new_block = tmp_block;
    }    
}


void sbmem_remove (){
    if(shm_unlink ("/sharedMem")){
        printf("Removed successfully");
    }else{
        printf("Error in remove");
    }
}


int sbmem_open(){

    fd = shm_open("/sharedMem",O_RDWR , 0666 );   
    page_addr =(struct block *) mmap(0,32768,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0); 
    if(page_addr == MAP_FAILED){
       printf( "Mmap failed: \n");
       return -1;
    }
    SEG_SIZE = page_addr->limit;
    if(page_addr->no_active_process > 10){
        printf("Processes exceeded the limit");
        return -1;
        
    }
    page_addr = (struct block *) mmap(0,SEG_SIZE,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
    if(page_addr == MAP_FAILED){
       printf( "Mmap failed: \n");
       return -1;
    }
 
    page_addr->no_active_process++;
    printf("Opened library for allocation \npid :%s ",getpid());
    return 0;

}





 void *sbmem_alloc (int reqsize){
    int realsize = 8 + reqsize;
    if(realsize > 4096 ||realsize < 128 ){
        return NULL;
    }
    struct  block * tmp = p_map;

    struct  block * deleted_target;
    
    while(tmp != NULL && tmp->next->limit != reqsize){tmp = tmp->next;}
    
    if (tmp == NULL){
        return NULL;
    }

    deleted_target = tmp->next;
    tmp->next = deleted_target->next;
    deleted_target->next = NULL;
    return deleted_target;



 }



void sbmem_free (void *ptr){

}



int sbmem_close (){

}