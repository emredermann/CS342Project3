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

/* COmmands
gcc -o create sbmemlib.c create_memory_sb.c  -lm -lrt -lpthread
gcc -o destroy sbmemlib.c destroy_memory_sb.c  -lm -lrt -lpthread
*/



struct block{
    int address;
    int limit;
    int no_active_process;
    struct block * next;
};

struct block * page_addr;
struct block * p_map;
const char * sharedMemName = "/sharedMem";
const int minimum_segsize = 32768;
int virtualAddress;
int SEG_SIZE;
int fd;
int pid = -1;
//sem_t mutex;
 
struct block*  combineBlocks(struct  block * ptr_1,struct  block * ptr_2);
struct  block* DivideBlock( int realsize);
void linkedlistInit(struct block * target);
int nextPower(int num);



int sbmem_init (int segsize){

    //sem_init(&mutex,1,0);
    
    printf("*********************************************** \n");
    SEG_SIZE = segsize;
    if (shm_unlink( sharedMemName ) != -1)
    {
        printf("Shared memory unlinked.");
        
    }
    printf("İlk İf*************************************** \n");
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
   printf("Before truncate************************************** \n");
    if( ftruncate( fd,  segsize) == -1 ) {
        printf("ftruncate error \n");
        return -1;
    }
    printf("Aftert truncate************************************** \n");
    
    //Initializes the shared memory mapps it to the p_map
    printf("Before mmap*************************************** \n");
    p_map = (struct block *) mmap( NULL, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    if(p_map == MAP_FAILED){
        printf( "Mmap failed !!: \n");
        return -1;
    } printf("after mmap*************************************** \n");

    p_map->limit = SEG_SIZE;
    printf("pmap limitim budur abim %d \n", p_map->limit);
    printf("after limit*************************************** \n");
    p_map -> address = 0;
    printf("after address*************************************** \n");
    p_map -> no_active_process = 0;
    printf("after activeprocess*************************************** \n");
  
    
    printf("before init linkedlist");
   
    printf("After init linkedlist before semaphore");
    
    //sem_post(&mutex);
    printf("after init linkedlilst");
    return 0;
    
}


void linkedlistInit(struct block * target){

    int i  = 7;
    printf("newblock init linkedlist");
    struct block * new_block;
    new_block->limit= pow(2,i);
    new_block->address = 8;
     
    target->next = new_block;
    printf("Inside linkedlist INIT");
    while(pow(2,i)< SEG_SIZE){
        
        struct block * tmp_block;
        tmp_block->limit = pow(2,i);
        tmp_block->address = new_block->address + new_block->limit;
        i++;
        new_block->next = tmp_block;
        new_block = tmp_block;
    }    
}

 
void sbmem_remove(){
    //sem_wait(&mutex);
     printf("REMOVE A BAŞLIYORUZ*************************************** \n");
    if(shm_unlink (sharedMemName) == 0){
        printf("Removed successfully");
    }else{
        printf("Error in remove");
    }
     printf("REMOVE BİTTİ************************************ \n");
    //sem_post(&mutex);
}
 
int sbmem_open(){

    //sem_wait(&mutex);
    fd = shm_open("/sharedMem",O_RDWR , 0666 );
    
    printf("OPENA GİRDİK YEĞEN\n");
    page_addr = (struct block *) mmap(0, minimum_segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    //int * test = (int * ) mmap(0, minimum_segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    //int testsize = test[0];
    //printf("test size %d\n",testsize);
    //int testaddress = test[1];

    printf("PAGE ADDRESSİ ALDIK YEĞEN\n");
    
    if(page_addr == MAP_FAILED){
       printf( "Mmap failed: \n");
       return -1;
    }
    
    printf("MAP FAİL ETMEDİ YEĞEN\n");
    
    SEG_SIZE = page_addr->limit;
    printf("segsize %d\n", SEG_SIZE);
    if(page_addr->no_active_process > 10){
        printf("Processes exceeded the limit");
        return -1;
        
    }
    
    int x = munmap(page_addr, minimum_segsize);
    if(x == -1){
       printf( "Munmap failed: \n");
       return -1;
    }
    
    page_addr = (struct block *) mmap(0,SEG_SIZE,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
    
    if(page_addr == MAP_FAILED){
       printf( "Mmap failed: \n");
       return -1;
    }
    
    printf("PAGE ADDRESS BUDUR YEĞEN %d \n", page_addr);
    
    printf("MAP YİNE FAİL ETMEDİ YEĞEN\n");
    //**********************
    //linkedlistInit(page_addr);
    
//**********************LINKED LIST INITIALIZATION BEGINNING
	
	struct block * target = page_addr;
	printf("TARGET POINTERI TUTTUK YEĞEN\n");
	printf("TARGET POINTERI TUTTUK YEĞEN\n");
	
	
	int i  = 7;
	printf("newblock init linkedlist\n");
	//int limit = pow(2,i);
	//int address = 0;
	//int next = limit + address;
	
	
	printf("target + target->limit %d\n", target + target->limit);
	struct block * new_block = target + target->limit;
	new_block->limit= pow(2,i);
	new_block->address = 0;
	
	
	target->next = new_block;
	printf("Inside linkedlist INIT\n");
	while(pow(2,i)< SEG_SIZE){

		struct block * tmp_block = new_block + new_block->limit;
		tmp_block->limit = pow(2,i);
		tmp_block->address = new_block->address + new_block->limit;
		i++;
		new_block->next = tmp_block;
		new_block = tmp_block;
	} 
	 
	 
	 


//**********************LINKED LIST INITIALIZATION END
    
    printf("LİNKED LİSTİ KURDUM YEĞEN\n");
    //sem_post(&mutex);

    pid = getpid();
    printf("pid %d\n", pid);
    page_addr->no_active_process++;
    printf("Opened library for allocation pid :%d \n",getpid());
    return 0;
}


void *sbmem_alloc (int reqsize){
	//sem_wait(&mutex);
    if(pid == -1){
        printf("U can not alloc before open in shared memory.");
        //sem_post(&mutex);
        return NULL;
    }   
    int realsize = nextPower(12 + reqsize);

    if(realsize > 4096 ||realsize < 128 ){
        //sem_post(&mutex);
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
        //sem_post(&mutex);
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
        //sem_post(&mutex);
        return deleted_target;
 }

struct block* DivideBlock( int realsize){
    	//sem_wait(&mutex);
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


void sbmem_free(void *ptr){
    	//sem_wait(&mutex);
    	struct  block * deleted_target = (struct block *) ptr;
    	if(pid == -1){
            printf("U can not alloc before open in shared memory.");
            return;
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
    //sem_post(&mutex);
}
struct block*  combineBlocks(struct  block * ptr_1,struct  block * ptr_2){
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
