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
#define SEGMENT_SIZE 512

//https://www.geeksforgeeks.org/buddy-memory-allocation-program-set-1-allocation/
//https://www.tutorialspoint.com/inter_process_communication/inter_process_communication_shared_memory.htm

// Important One
//https://stackoverflow.com/questions/29238015/creating-shared-memory-segments

//mmap implementation
//https://yandex.com/turbo/devsday.ru/s/blog/details/21607

//About mmap and shm_open for sharing memory between processes
//https://stackoverflow.com/questions/4991533/sharing-memory-between-processes-through-the-use-of-mmap
//https://stackoverflow.com/questions/15029053/share-process-memory-with-mmap

// Shared memory object
// The attributes or the structure must be checked. 
typedef struct {
    pid_t pid;
    // Current location of the block 
    int baseAddress;
    // Limit of the block
    int limit;
    block * next;
    block * rear;
    block * head;
}block;


block * head;

block * current_pointer;
block * p_map;
int current_counter;
int fd;
int pid;
int counter;
bool activeProcess;
int virtualAddress;
sem_t mutex;



/*
Creates the 2 dimesional linkedlists one dimension.
*/
void linkedlistInit(block* target){
    //i =  means 512 memory size
    block * target;
    target->rear = NULL;
    target->limit = 2;
    // Address ??
    //tmpNode->baseAddress =
    head = target;

    for (int i = 2; i < 10; i++)
    {
        block * newNode;
        newNode->limit = pow(2,i);
        newNode->next = NULL;
        newNode->rear = target;
        newNode->head = NULL;
        target->next = newNode;
        target = target->next;
    }
    
    printf("Doubly linkedlist created succesfully");
    
}


/*
returns the pointer of the target memory space segment.
Example:
If the target seg size is 30 it returns the head of the 32.
*/
block* freeHeadPointerLocator(int segSize){
     block *tmp = head;
     while(tmp != NULL){
         if(tmp->limit = nextPower(segSize)){
             return tmp;
         } 
         tmp = tmp->next;
     }
     return NULL;
}


block * freeNodeAllocator(block * head,int segSize){
    block * tmp = head;
    if(tmp != NULL){
        //deallocate the tmp and return the deallocated tmp as a free space must be used.
        return tmp;
    }
    //Means there is no free space
    return NULL;
}


/*
    Creates a new space according to the bigger egment size linkedlist situation.

void createNewFreeSpace(block * headOfTheNextNode){
    
    block * tmp = head;
    while(tmp->next != headOfTheNextNode)
        tmp = tmp->next;

    //Deallocate - delete one node from headOfTheNextNode and create 2 node to the tmp.
        block * current =tmp->next->head;
        while (current->next != NULL ){
              current = current->next; 
        }
        if(current == tmp->next->head){
            // Means empty linkedlist
        }else{
            // Deletes one (where current node pointer is) node from the original linkedlist.
            current = current->rear;
            tmp = current->next;
            current->next = NULL;
            delete(tmp); 
        }
       
    // Newly created nodes for the one lower segment.
    block * newNode,* secondNewNode;
    
    newNode->limit = tmp->limit;
    newNode->rear = tmp->head;
    tmp->head->next = newNode;
    
    secondNewNode->limit = tmp->limit;
    secondNewNode->next = NULL;
    secondNewNode->rear = newNode;
    newNode->next = secondNewNode;
}
*/


/*
    Creates a new space according to the bigger segment size linkedlist situation.
*/
void createNewFreeSpace(block * headOfTheNextNode){
    
    block * tmp = head;
    while(tmp->next != headOfTheNextNode) {
        tmp = tmp->next;
	}

    //Deallocate - delete one node from tmp->head and create 2 nodes at tmp->next.
	block * current = tmp->head;

	// Deletes one (where current node pointer is) node from the original linkedlist.
	current->next->rear = NULL;
	tmp->head = current->next;
	current->next = NULL;
	delete(current); 
	current = NULL;
	
	tmp = tmp->next;

    // Newly created nodes for the next segment.
    block * newNode,* secondNewNode;
    
    newNode->limit = tmp->limit;
    newNode->rear = NULL;
	newNode->pid = getpid();
	newNode->address = newNode;
    tmp->head = newNode;
    
    secondNewNode->limit = tmp->limit;
	secondNewNode->pid = getpid();
	secondNewNode->address = secondNewNode;
    secondNewNode->next = NULL;
    secondNewNode->rear = newNode;
    newNode->next = secondNewNode;
}

/*
	Deallocates buddy node from free space list and allocated to shared memory
*/
block *allocateBuddyNodeToSharedMem(block * ptr){
    block * tmp = ptr->head;
	
    if(tmp == NULL){
		//no free space
		return NULL;
    }
	
	ptr->head = ptr->head->next;
	ptr->head->rear = NULL;
	
	space_allocated += tmp->limit;
	free_space -= tmp->limit;
	
	//Add node to p_map linked list
	if (p_map == NULL){ //if empty list add directly to head
		p_map = tmp;
		tmp->next = NULL;
		tmp->rear = NULL;
	}
	else {
		tmp->next = p_map->next;
		tmp->rear = NULL;
		p_map = tmp;
	}
	
	return tmp; //Burada p_map returnlemek daha mantıklı olabilir
    
} 

void *sbmem_alloc (int reqsize){
	
	int realsize = 8 + reqsize;
 
    block *ptr = freeHeadPointerLocator(realsize);
	block *tmp = head;
	int required_size = nextPower(realsize);
	
	if (ptr->head == NULL && free_space == 0) {
		printf("Memory could not be allocated");
		return NULL;
	}
	else if (ptr->head == NULL && free_space > realsize) {
		
		while (tmp != ptr) {
			if (free_space >= tmp->limit && tmp->head != NULL){
				createNewFreeSpace(tmp);
			}
			tmp = tmp->next;
		}
	}
	
	tmp = allocateBuddyNodeToSharedMem(ptr);
	
	return tmp;
    
}

int sbmem_init(int segsize){
    pid = 0;
    activeProcess = false;
    virtualAddress = 0;
    sem_init(&mutex, 1, 0);
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
    if( ftruncate( fd, sizeof( nextPower(segsize) * 9) ) == -1 ) {
        printf("ftruncate error \n");
        return -1;
    }
    //Initializes the shared memory mapps it to the p_map
    // Size of the mapped segment.
    p_map = (block *) mmap( virtualAddress, sizeof(block) * 9, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    virtualAddress += sizeof(block) * 9;
    linkedlistInit(p_map);
    
    if(p_map == MAP_FAILED){
        printf( "Mmap failed: \n");
        return -1;
    }
    return 0;
}



// Have doubts about 
// The created semaphore(s) will be removed as well. 
bool sbmem_remove (){
   sem_wait(&mutex);
   sem_post(&mutex);
}


int sbmem_open(){
    if(activeProcess){
        return -1;
    }
    sem_wait(&mutex);
    //What is was the reason of the virtual address.
    //What was the aim?
    
    current_pointer = (block *) mmap( virtualAddress, sizeof(block), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    current_pointer->baseAddress = virtualAddress;
    current_pointer->pid = pid;
    activeProcess = true;
    pid ++;
    sem_post(&mutex);
    return 0;
}

/*
void *sbmem_alloc (int reqsize){
 
    if (p_map != NULL){
         return p_map;
        }
    printf("Memory could not allocated");
    return NULL;
}
*/

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
    sem_wait(&mutex);
    pid--;
    free(ptr);
    activeProcess = false;
    sem_post(&mutex);
}
 
 int sbmem_close(){
     sem_wait(&mutex);
    activeProcess = false;
    sem_post(&mutex);
    if(shm_unlink("/sharedMem"))
        return 1;
    return 0;
 }
