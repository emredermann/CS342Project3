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
 
void combineBlocks();
struct  block* DivideBlock( int realsize, int max);
void printList();

struct block{
	int location;
	int limit;
	int next;
};

struct  block* DivideBlock( int realsize,int max);
int nextPower(int num);
int checkLocationStartingPoint(int location);
void combineBlocks();
void * page_addr;
struct block * p_map;
struct block * list_head;
const char * sharedMemName = "/sharedMem";
const int minimum_segsize = 32768;
int virtualAddress;
int SEG_SIZE;
int fd;
int management_size;
int pid = -1;
sem_t mutex;


void linkedlistInit(){

	void * ptr = page_addr;
	void * head = page_addr + sizeof(struct block);
	int count = 0;
	int init_segsize = SEG_SIZE;
	
	while(init_segsize > management_size){
		init_segsize = init_segsize /2;
		count ++;
	}
	
	int tmpa = management_size;
	int tmpb = management_size * 2;
	struct block * tmp = (struct block *) head;
	for (int i = 0; i < count -1; i++){

		tmp->location = tmpa;
		tmp->limit = tmpa;
		tmp->next = tmpb;
		
		printf("%d,%d,%d \n",tmp->location,tmp->limit,tmp->next);

		tmpa = tmpb;
		tmpb = tmpb *2;
		head = head + sizeof(struct block);
		tmp = (struct block *) head;
	}
	tmp->limit = tmpa;
	tmp->location = tmpa;
	tmp->next = -1;

}

int checkLocationStartingPoint(int location){
	int n = 1;
	while (n < 12){
		if (location == pow(2,n)){
			return 0;
		}
		n++;
	}
	return -1;
}

void printList(){
	void * tmp = page_addr;
	tmp = tmp + sizeof(struct  block);
	struct block * tmp2 = tmp;

	while(tmp2->next != -1  ){

		printf("%d,%d,%d \n",tmp2->location,tmp2->limit,tmp2->next);
		tmp = tmp + sizeof(struct block);
		tmp2 = tmp;
	}
	printf("%d,%d,%d \n",tmp2->location,tmp2->limit,tmp2->next);

}

int sbmem_init (int segsize){
	
	printf("-----------------SBMEM-INIT----------------\n\n");
	printf("Hello. Process %d has started intializing the shared memory opening process.\n", getpid());
	SEG_SIZE = segsize;
	
	if (shm_unlink( sharedMemName ) != -1)
	{
		printf("Shared memory unlinked.\n");
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

	management_size = nextPower((int) (((segsize / 256) / 2) + 3) * sizeof(struct block));
	//Initializes the shared memory maps it to the page_addr
	page_addr = mmap( NULL, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );

	if(page_addr == MAP_FAILED){
		printf( "Mmap failed !!: \n");
		return -1;
	} 

	p_map = (struct block *) page_addr;
	p_map->limit = SEG_SIZE;
	p_map->location = 0;

	// Active processor
	p_map->next = 1;
	list_head = p_map + sizeof(struct block);
	
	linkedlistInit();
	printList();
	
	printf("-----------------SBMEM-INIT----------------\n\n");
	return 0;
    
}

void sbmem_remove(){
     
	printf("-----------------SBMEM-REMOVE----------------\n\n");
	printf("Process %d has called the shared memory removal process.\n", getpid());
	if (shm_unlink (sharedMemName) == 0){
		printf("Shared memory removed successfully.\n");
	} 
	else {
		printf("Error in removing shared memory.\n");
	}
	printf("-----------------SBMEM-REMOVE----------------\n\n");
}
 
int sbmem_open(){
	
	printf("-----------------SBMEM-OPEN----------------\n\n");
	printf("Process %d has requested to open the shared library for allocation.\n", getpid());
	sem_init(&mutex,1,1);
	sem_wait(&mutex);
	fd = shm_open(sharedMemName,O_RDWR , 0666 );

	page_addr = mmap(0, 32768, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

 
	if(page_addr == MAP_FAILED){
		printf( "Mmap failed: \n");
		sem_post(&mutex);
		return -1;
	}
	struct block * tmp = (struct block *) page_addr;
	
	

 
	SEG_SIZE = tmp->limit;

	if(tmp->next > 10){
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

  
	pid = getpid();
	printf("pid %d\n", pid);
	tmp->next = tmp->next + 1;
	
 	sem_post(&mutex);
 	printf("-----------------SBMEM-OPEN----------------\n\n");
	return 0;
}

void *sbmem_alloc (int reqsize){

	printf("-----------------SBMEM-ALLOC----------------\n\n");
	printf("Process %d has requested to allocate %d bytes of space.\n", getpid(), reqsize);
	printf("The real size of this rquest is %d as there is an additional overhead of 12 bytes.\n", reqsize + 12);
	
	sem_wait(&mutex);
	if(pid == -1){
		printf("Can not alloc before open in shared memory.\n");  
		return NULL;
	}   
    
    
	int realsize = nextPower(12 + reqsize);
	printf("Internal fragmentation for this allocation is %d bytes since actual size allocated for request is %d.\n",realsize-reqsize, realsize);

	if(realsize > 4096 ||realsize < 128 ){
		return NULL;
	}

	void * tmp = page_addr + sizeof(struct block);
	struct  block * new_tmp = (struct block *) tmp;

	int max = -1;

	while( (new_tmp->next != -1 ) && (new_tmp->limit  != realsize) ){
		if(max == -1 && new_tmp->limit >= realsize){
			max = new_tmp->limit;
		}
		else if ((new_tmp->limit >= realsize) && max > new_tmp->limit ) {
			max = new_tmp->limit;
		}
		tmp = tmp + sizeof(struct block);
		new_tmp = tmp;
	}


	// Linkedlist occurs as single node
	if(max == -1){
		if(new_tmp->limit >= realsize){
			max = new_tmp->limit;
		}
		else{
			sem_post(&mutex);
			printf("No free space.");
			return NULL;        
		}
	}

	if(new_tmp->limit == realsize){    
		struct block * tmp_next;

		// Takes the result pointer 
		struct block * result = new_tmp; 
		tmp_next = page_addr + new_tmp->location;

		// Left shift operation
		while(tmp_next->next != -1){
			new_tmp->limit = tmp_next->limit;
			new_tmp->location = tmp_next->location;
			new_tmp->next = tmp_next->next;
			new_tmp = page_addr + sizeof(struct block);
			tmp_next = tmp_next + sizeof(struct block);
		}     

		tmp_next = tmp_next + sizeof(struct block);
		result->next = -2;
		sem_post(&mutex);
		return (void *) result;
	}
    
     
	sem_post(&mutex);
   
    	printList();
    	printf("Dividing blocks after allocation.\n");
    	
    	return DivideBlock (realsize,max);

}

struct block * DivideBlock( int realsize,int max){
     
	void * cur = page_addr + sizeof(struct block);
	struct block * cur_ptr = (struct block *) cur;
	
	while (cur_ptr->limit != max)
        { 
        	cur = cur + sizeof(struct block);
        	cur_ptr = cur;
        }
        
        void * tmp = cur;
	struct block * tmp_ptr = (struct block *)tmp;
	
        
        //Tmp_ptr is the last node
        while(tmp_ptr->next != -1){
        	tmp = tmp + sizeof(struct block);
        	tmp_ptr = tmp;
        }
        tmp = tmp + sizeof(struct block);
        tmp_ptr = tmp;
        
  
        struct  block * new_tmp = tmp_ptr;
        
        new_tmp->next = -1;
        new_tmp->location = tmp_ptr->location;
        tmp += sizeof(struct block);
        tmp_ptr = tmp;
   
	void * holder;
        struct block * tmp_ptr_next;
        while ( tmp_ptr != cur_ptr)
        {   
        	holder = tmp + sizeof(struct block);
        	//tmp = holder;
        	
		// Set the tmp_ptr_next as new created space for shifting initially.
		tmp_ptr_next = holder;

		// Shifted all the elements to the right starting from last node to cur. 
		tmp_ptr_next->limit = tmp_ptr->limit;

		// Opens location for the clonned struct 
		((struct block *)tmp_ptr_next)->location = ((struct block *)tmp_ptr)->location + sizeof(struct block);
		((struct block *)tmp_ptr_next)->next = ((struct block *)tmp_ptr)->next;
		
		// As a last copy process the cur node will be copied to two consecutive nodes
		tmp -= sizeof(struct block);
		tmp_ptr = tmp;
        }
         
        int tmp_location = ((struct block *)cur)->location;
        int tmp_limit = ((struct block *)cur)->limit;
        int tmp_next = ((struct block *)cur)-> next;

	//Set the size of the created nodes.
	((struct block *) tmp_ptr) -> limit = tmp_limit / 2;
	((struct block *) tmp_ptr_next) -> limit = tmp_limit / 2;
	//Lower location variable returned.

	printList();
	printf("-----------------SBMEM-ALLOC----------------\n\n");
	return ((struct block *) tmp_ptr);
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
	printf("-----------------SBMEM-FREE----------------\n\n");
	printf("Process %d has requested to free %d bytes of space.\n", getpid(), ((struct block *)ptr)->limit);

	sem_wait(&mutex);
 
	struct block * page_ptr = (struct block *)page_addr;

	if(page_ptr->next == -1){
		printf("U can not alloc before open in shared memory.");
		sem_post(&mutex);
		return;
	}   

 	//Linkedlist pointer
	void * voidCurPtr = page_addr + sizeof(struct block);
	struct  block * current_ptr = (struct block *) voidCurPtr;

 
	//Pointer location
	int val = nextPower((int) (((SEG_SIZE / 256) / 2) + 3) * sizeof(struct block));

	//Ptr
	void * blockTmpPtr = page_addr + val;
	struct block * tmp = (struct block *) blockTmpPtr;

 
	//mem location to be freed
	void * blockCurPtr = blockTmpPtr+ sizeof(struct block);
	struct block * cur = (struct block *) blockCurPtr;

 
	int mode = 1;

	if(current_ptr->location == val){
		mode = 0;
	}
	
	while(cur != ptr){


		if( mode == 0){
 			if(((current_ptr->location) + (current_ptr->limit)) == current_ptr->next) {

				val = val + current_ptr->limit;
				voidCurPtr = voidCurPtr + sizeof(struct block);
				current_ptr = voidCurPtr;

			}
			else {
				val = val + current_ptr->limit;
				blockTmpPtr = page_addr + val;
				tmp = blockTmpPtr;
				mode = 1;
				blockCurPtr = tmp + sizeof(struct block);
				cur = blockCurPtr;
			}
		}
		else {      
 			if((val + (tmp->limit)) == current_ptr->next || (val + (tmp->limit)) == current_ptr->location){
				val = val + tmp->limit;
				mode = 0;
				voidCurPtr = voidCurPtr + sizeof(struct block);
				current_ptr = voidCurPtr;
			}else {
				val = val + tmp->limit;
				blockTmpPtr = page_addr + val;
				tmp = blockTmpPtr;
				blockCurPtr = tmp + sizeof(struct block);
				cur = blockCurPtr;
			}
		}

		if(current_ptr->location == -1){
			printf("Could not founded");
			sem_post(&mutex);
			return;
		}

	}

	//if there are no free nodes
	if(current_ptr->next == -2){
  		struct block * newNode;
		newNode->limit = ((struct block * )ptr)->limit;
		newNode->location =  ((struct block * )ptr)->location;
		newNode->next = -1;
		return ;
	}

	//if there is only one free node
 	else if (current_ptr->next == -1)
	{
 		struct block * newNode;
		newNode->limit = ((struct block * )ptr)->limit;
		newNode->location =  ((struct block * )ptr)->location;
		if(newNode->location <  current_ptr->location){
			newNode->next = current_ptr->location;
			list_head = newNode;
		}else{
			newNode->next = -1;
			current_ptr->next = newNode->location;
		}
		combineBlocks();       
	}
 	//if there are more than one free node
	else{
 		struct block * shift_pointer = current_ptr;
		void * voidShiftPtr = shift_pointer;

		while(shift_pointer->next != -1){
			voidShiftPtr = voidShiftPtr +sizeof(struct block);
			shift_pointer = voidShiftPtr;
		}

		voidShiftPtr = voidShiftPtr +sizeof(struct block);
		struct block * newNode = voidShiftPtr;

		newNode->next = -1;
		newNode->limit = shift_pointer->limit;
		newNode->location = shift_pointer->next;

		while(shift_pointer->location != current_ptr->location){
			int nextLocation = shift_pointer->location;
			newNode = shift_pointer;

			voidShiftPtr = voidShiftPtr - sizeof(struct block);
			shift_pointer = voidShiftPtr;

			newNode->limit = shift_pointer->limit;
			newNode->location = shift_pointer->next;
			newNode->next = nextLocation;
		}
		voidShiftPtr = voidShiftPtr +sizeof(struct block);
		shift_pointer = voidShiftPtr;

		current_ptr->next = ((struct block * )ptr)->location;
		((struct block * )ptr)->next = shift_pointer->location;
		combineBlocks();

	}
        
        printf("-----------------SBMEM-FREE----------------\n\n");
        
}

void combineBlocks(){
	
	bool go_through_list_again = false;

	do{
		struct block * cur = (struct block *) page_addr;
		cur = cur + sizeof(struct block);
	
		struct block * cur_next = cur + sizeof(struct block);
		go_through_list_again = false;

		//Traverse through whole list; go_through_list_again = false, no need to combine blocks; otherwise, traverse and check again
		while(cur->next != -1){

			if(checkLocationStartingPoint(cur->location) == 0){ // if the location of temp starts at 2^n

				if(cur->location + cur->limit == cur_next->location && cur->limit == cur_next->limit){ 


					cur->limit = cur->limit * 2;
					cur->next = cur_next->next;
					go_through_list_again = true; // Signal for repetition

					struct block * nextN = cur_next + sizeof(struct block);

					while(cur_next->next != -1){

						cur_next->location = nextN->location;
						cur_next->limit = nextN->limit;
						cur_next->next = nextN->next;
						nextN = nextN + sizeof(struct block);
						cur_next = cur_next + sizeof(struct block);
					}

					break;

				}
			}

		    	// Continue
		    	cur = cur + sizeof(struct block);
		    	cur_next = cur_next + sizeof(struct block);

		}




	} while(go_through_list_again != false);  
    
}

int sbmem_close (){
    
	printf("-----------------SBMEM-CLOSE----------------\n\n");
	printf("Process %d has requested to close the shared library to allocation.\n", getpid());
	int t = munmap(page_addr, SEG_SIZE);
	struct block *ptr = (struct block *)page_addr;
	ptr->next--;
	printf("To use the library again first call sbmem_open()\n");
	printf("-----------------SBMEM-CLOSE----------------\n\n");
   
}
