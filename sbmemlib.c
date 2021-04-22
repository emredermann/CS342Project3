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
int pid = -1;

 
struct block*  combineBlocks(struct  block * ptr_1,struct  block * ptr_2);
struct  block* DivideBlock( int realsize);
void linkedlistInit(struct block * target);
int nextPower(int num);



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
    p_map -> limit = SEG_SIZE;
    p_map -> address = 0;
    p_map -> no_active_process = 0;

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
    new_block->limit= pow(2,i);
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

/*
void sbmem_remove(){
    
    if(shm_unlink ("/sharedMem")){
        printf("Removed successfully");
    }else{
        printf("Error in remove");
    }
    
}
*/
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
    pid = getpid();
    page_addr->no_active_process++;
    printf("Opened library for allocation \npid :%s ",getpid());
    return 0;
}


void *sbmem_alloc (int reqsize){

    if(pid == -1){
        printf("U can not alloc before open in shared memory.");
        return NULL;
    }   
    int realsize = nextPower(12 + reqsize);

    if(realsize > 4096 ||realsize < 128 ){
        return NULL;
    }
    struct  block * tmp = page_addr;
    bool tmp_size = false;
    struct  block * deleted_target;
    
    while((tmp != NULL) && (tmp->next->limit != realsize )){
        if(tmp->next->limit  > realsize){
            tmp_size = true;
        }
        tmp = tmp->next;
    }
    
    if (tmp == NULL && tmp_size == false){
        printf("No free space.");
        return NULL;
    }else if (tmp == NULL && tmp_size == true)
     {
        struct block * ptr;
        do{
            ptr = DivideBlock(realsize);        
        }while(ptr->limit > nextPower(realsize));

    }
        deleted_target = tmp->next;
        tmp->next = deleted_target->next;
        deleted_target->next = NULL;
        return deleted_target;
 }

struct  block* DivideBlock( int realsize){
        struct  block * cur = page_addr;
        while (cur->next->limit <= realsize){cur = cur->next;}
        
        int tmp_address = cur->next->address;
        int tmp_limit = cur->next->limit;
        struct block * tmp_next = cur->next->next;
        
        struct block * tmp_delete = cur->next;

        struct  block * new_block_1;
        struct  block * new_block_2;
        
        new_block_1->limit = tmp_limit / 2;
        new_block_2->limit = tmp_limit / 2;
        new_block_1->address = tmp_address;
        new_block_2->address = tmp_address + (tmp_limit / 2);

        cur->next = new_block_1;
        new_block_1->next = new_block_2;
        new_block_2->next = tmp_next;
        delete(tmp_delete);
        return new_block_1;
    
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

struct  block * deleted_target = (struct block *) ptr;
 if(pid == -1){
        printf("U can not alloc before open in shared memory.");
        return NULL;
    }   
    
    struct  block * tmp = page_addr;
    while(tmp != NULL && tmp->next <= deleted_target->limit){tmp = tmp->next;} 
        
    deleted_target->next = tmp->next;
    tmp->next = deleted_target;
    struct block * block_to_be_combined = deleted_target;


// Infinite loop bak!!
    while(block_to_be_combined->limit == block_to_be_combined->next->limit)
    {
        block_to_be_combined = combineBlocks(block_to_be_combined,block_to_be_combined->next);
    }
    
}
struct block*  combineBlocks(struct  block * ptr_1,struct  block * ptr_2){
   
    struct block * combined_block;
    combined_block->address = ptr_1->address;
    combined_block->limit = ptr_1->limit * 2;
    combined_block->next = ptr_2->next;
    delete(ptr_1); delete(ptr_2);
    ptr_1 = NULL;ptr_2 = NULL;
    return combined_block;

    
        
}





int sbmem_close (){
    page_addr->no_active_process--;
    int t = munmap(page_addr, SEG_SIZE);
    printf("To use the library again first call sbmem_open()");
    if(shm_unlink("/sharedMem"))
        return 1;
    return 0;
}