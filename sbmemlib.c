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

/* Commands
gcc -o create sbmemlib.c create_memory_sb.c  -lm -lrt -lpthread
gcc -o destroy sbmemlib.c destroy_memory_sb.c  -lm -lrt -lpthread
*/



struct block{
    int location;
    int limit;
    int next;
    int no_active_process;
};

void * page_addr;
struct block * p_map;
const char * sharedMemName = "/sharedMem";
const int minimum_segsize = 32768;
int virtualAddress;
int SEG_SIZE;
int fd;
int pid = -1;
sem_t mutex;
 
 


int sbmem_init (int segsize){

    printf("*********************************************** \n");
    SEG_SIZE = segsize;
    if (shm_unlink( sharedMemName ) != -1)
    {
        printf("Shared memory unlinked.");
        
    }

    fd = shm_open(sharedMemName,O_RDWR | O_CREAT, 0666 );

    printf("Shared memory linked. \n");
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
    printf("Before mmap*************************************** \n");
    void * ptr =  mmap( NULL, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    
    if(p_map == MAP_FAILED){
        printf( "Mmap failed !!: \n");
        return -1;
    } printf("after mmap*************************************** \n");

    p_map = (struct block *) ptr;
    p_map->limit = SEG_SIZE;
    p_map->location = 0;
   
    
    return 0;
    
}


void linkedlistInit(){

    int i  = 7;
    void * ptr = page_addr;
    struct block * new_block ;
    
    new_block->location = ptr;
    new_block->next = ptr + sizeof(struct block);
    ptr = ptr + sizeof(struct block);

    
    int limit = pow(2,i);
    new_block->limit = limit;    
    
     
    while(pow(2,i)< SEG_SIZE){
        
        struct block * tmp_block;
        tmp_block->limit = pow(2,i);
        tmp_block->location = ptr;
        i++;
        tmp_block->next = ptr + sizeof(struct block);
        ptr = ptr  + tmp_block->limit;
    }    
}

 
void sbmem_remove(){
     
    if (shm_unlink (sharedMemName) == 0){
        printf("Removed successfully");
    } 
    else {
        printf("Error in remove");
    }
}
 
int sbmem_open(){

    sem_init(&mutex,1,1);
    sem_wait(&mutex);
    fd = shm_open(sharedMemName,O_RDWR , 0666 );
    
     
    page_addr = mmap(0, minimum_segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
 
    
    if(page_addr == MAP_FAILED){
       printf( "Mmap failed: \n");
       sem_post(&mutex);
       return -1;
    }
    struct block * tmp = (struct block *) page_addr;
    
    
    SEG_SIZE = tmp->limit;

    if(tmp->no_active_process > 10){
        printf("Processes exceeded the limit");
        sem_post(&mutex);
        return -1;
        
    }
    
    
    page_addr =  mmap(0,SEG_SIZE,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
    
    if(page_addr == MAP_FAILED){
       printf( "Mmap failed: \n");
       sem_post(&mutex);
       return -1;
    }
    
    printf("Page address is : %d \n", page_addr);
    
    
    //**********************
    
    linkedlistInit();

    printf("Linked list called safely. \n");

    pid = getpid();
    printf("pid %d\n", pid);
    tmp->no_active_process++;
    printf("Opened library for allocation pid :%d \n",getpid());
    sem_post(&mutex);
    return 0;
}


void *sbmem_alloc (int reqsize){
	sem_wait(&mutex);
    if(pid == -1){
        printf("U can not alloc before open in shared memory.");  
        return NULL;
    }   
    int realsize = nextPower(12 + reqsize);

    if(realsize > 4096 ||realsize < 128 ){
        return NULL;
    }
    
     
    struct  block * tmp = (struct block *) page_addr;
    struct  block * deleted_target;

    // En uygun size
    int max = -1;

    /*
        iSTEDĞİM SİZEDAN BÜYÜK EN KÜÇÜK & uygun olan size  
    */
    while( (tmp->next != -1 ) && (tmp->limit  != realsize) ){
        if(max == -1 && tmp->limit >= realsize){
            max = tmp->limit;
        }else if ((tmp->limit >= realsize) && max > tmp->limit )
        {
            max = tmp->limit;
        }
        tmp = tmp + sizeof(struct block);
    }
    
    // Linkedlist tek node ise
    if(max == -1){
        if(tmp->limit >= realsize){
            max = tmp->limit;
        }else{
               sem_post(&mutex);
                printf("No free space.");
                return NULL;        
        }
    }
    
    // Tam fit varsa gireceği alan pointer
    if(tmp->limit == realsize){    
            
        
        
        return ptr;
    }
    
    // Tam fit yok alan gireceği alan pointer
    
    /*
    if (tmp->limit !=){
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
        sem_post(&mutex);
        return deleted_target;
 */
 
 }

struct block* DivideBlock( int realsize){
    
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
   	//sem_post(&mutex);
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


void sbmem_free(void *ptr)
{
    	//sem_wait(&mutex);
    	struct  block * deleted_target = (struct block *) ptr;
    	if(pid == -1){
            printf("U can not alloc before open in shared memory.");
            return;
        }   
        
        struct  block * tmp = page_addr;
        while(tmp != NULL && tmp->next <= deleted_target->limit)
        {
        tmp = tmp->next;
        } 
            
        deleted_target->next = tmp->next;
        tmp->next = deleted_target;
        struct block * block_to_be_combined = deleted_target;


        // Infinite loop bak!!
        while(block_to_be_combined->limit == block_to_be_combined->next->limit)
        {
            block_to_be_combined = combineBlocks(block_to_be_combined,block_to_be_combined->next);
        }
        //sem_post(&mutex);
}


struct block * combineBlocks(struct block * ptr_1,struct block * ptr_2)
{
	//sem_wait(&mutex);
	struct block * combined_block;
	combined_block->address = ptr_1->address;
	combined_block->limit = ptr_1->limit * 2;
	combined_block->next = ptr_2->next;

	ptr_1 = NULL;ptr_2 = NULL;
	//sem_post(&mutex);
	return combined_block;

}


int sbmem_close (){
    //sem_wait(&mutex);
    page_addr->no_active_process--;
    int t = munmap(page_addr, SEG_SIZE);
    printf("To use the library again first call sbmem_open()");
    if(shm_unlink("/sharedMem")){
        //sem_post(&mutex);
        return 1;
        }
    //sem_post(&mutex);
    return 0;
}
