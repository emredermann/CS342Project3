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
       printf("linkedlistinit head->location %d\n",tmp->location );
       tmp->limit = tmpa;
       tmp->next = tmpb;
       printf("linkedlistinit head->next %d\n",tmp->next );
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
         printf("%d \n",management_size);
         tmp = tmp + sizeof(struct block);
         tmp2 = tmp;
     }
     printf("%d,%d,%d \n",tmp2->location,tmp2->limit,tmp2->next);

}

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
    
     management_size = nextPower((int) (((segsize / 256) / 2) + 3) * sizeof(struct block));
    //Initializes the shared memory maps it to the p_map
    printf("Before mmap*************************************** \n");
    page_addr = mmap( NULL, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    
    if(page_addr == MAP_FAILED){
        printf( "Mmap failed !!: \n");
        return -1;
    } printf("after mmap*************************************** \n");

	p_map = (struct block *) page_addr;
    	p_map->limit = SEG_SIZE;
    	p_map->location = 0;
    	
    	printf("YARAĞIM DAŞAĞIM 0\n");
    
    	// Active processor
    	p_map->next = 1;
    	list_head = p_map + sizeof(struct block);
    printf("YARAĞIM DAŞAĞIM 1\n");
    linkedlistInit();
    printf("YARAĞIM DAŞAĞIM 2\n");
    printList();
    return 0;
    
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
	
	printf("BÜLLÜK 1 \n");
	sem_init(&mutex,1,1);
	sem_wait(&mutex);
	fd = shm_open(sharedMemName,O_RDWR , 0666 );

	printf("BÜLLÜK 2 \n");
	
	page_addr = mmap(0, 32768, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	printf("BÜLLÜK 3 \n");

	if(page_addr == MAP_FAILED){
		printf( "Mmap failed: \n");
		sem_post(&mutex);
		return -1;
	}
	struct block * tmp = (struct block *) page_addr;
	
	

	printf("BÜLLÜK 4 \n");

	SEG_SIZE = tmp->limit;

	if(tmp->next > 10){
		printf("Processes exceeded the limit");
		sem_post(&mutex);
		return -1;

	}

	printf("BÜLLÜK 5 \n");

	page_addr =  mmap(0,SEG_SIZE,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);

	if(page_addr == MAP_FAILED){
		printf( "Mmap failed: \n");
		sem_post(&mutex);
		return -1;
	}

	printf("BÜLLÜK 6 \n");
	printf("Page address is : %d \n", page_addr);


	//**********************



	printf("Linked list called safely. \n");

	pid = getpid();
	printf("pid %d\n", pid);
	tmp->next = tmp->next + 1;
	
	printf("Opened library for allocation pid :%d \n",getpid());
	sem_post(&mutex);
	return 0;
}


void *sbmem_alloc (int reqsize){

	printf("ALLOC BÜLLÜK 1 \n");
	sem_wait(&mutex);
    if(pid == -1){
        printf("U can not alloc before open in shared memory.");  
        return NULL;
    }   
    
    
    int realsize = nextPower(12 + reqsize);

    if(realsize > 4096 ||realsize < 128 ){
        return NULL;
    }
    printf("ALLOC BÜLLÜK 2 \n");
     
    void * tmp = page_addr + sizeof(struct block);
    struct  block * new_tmp = (struct block *) tmp;

	printf("ALLOC BÜLLÜK 3 \n");
    // En uygun size
    int max = -1;

    /*
        iSTEDĞİM SİZEDAN BÜYÜK EN KÜÇÜK & uygun olan size  
    */
    while( (new_tmp->next != -1 ) && (new_tmp->limit  != realsize) ){
    	printf("new_tmp->next %d \n", new_tmp->next);
    	printf("ALLOC BÜLLÜK dfsdf \n");
        if(max == -1 && new_tmp->limit >= realsize){
            max = new_tmp->limit;
            printf("ALLOC BÜLLÜK if \n");
        }else if ((new_tmp->limit >= realsize) && max > new_tmp->limit )
        {
            max = new_tmp->limit;
            printf("ALLOC BÜLLÜK else \n");
        }
        printf("ALLOC BÜLLÜK dsdasas \n");
        tmp = tmp + sizeof(struct block);
        new_tmp = tmp;
    }
    
    printf("ALLOC BÜLLÜK 4 \n");
    
    // Linkedlist tek node ise
    if(max == -1){
        if(new_tmp->limit >= realsize){
            max = new_tmp->limit;
        }else{
               sem_post(&mutex);
                printf("No free space.");
                return NULL;        
        }
    }
    
    printf("ALLOC BÜLLÜK 5 \n");
    
    // Tam fit varsa gireceği alan pointer
    if(new_tmp->limit == realsize){    
        //new_tmp = new_tmp + sizeof(struct block);
        struct block * tmp_next;

        // Takes the result pointer 
        struct block * result = new_tmp; 
        tmp_next = page_addr + new_tmp->location;
      
      // Left shift operation
        while(tmp_next->location != -1){
            new_tmp->limit = tmp_next->limit;
            new_tmp->location = tmp_next->location;
            new_tmp->next = tmp_next->next;
            new_tmp = page_addr + sizeof(struct block);
            tmp_next = tmp_next + sizeof(struct block);
            }     
    
        tmp_next = tmp_next + sizeof(struct block);
        result->next = -2;
        return (void *) result;
    }
    
    	printf("ALLOC BÜLLÜK 6 \n");
    
	sem_post(&mutex);
    	// Tam fit yok ama gireceği alan var; gireceği alanın pointerını ver.
    	
    	printList();
    	return DivideBlock (realsize,max);

}





// Realsize = requested size + sizeof(struct block)

struct block * DivideBlock( int realsize,int max){

	printf("DİVİDEBLOCK BÜLLÜK 1 \n");
    
	void * cur = page_addr + sizeof(struct block);
	struct block * cur_ptr = (struct block *) cur;
	
	printf("DİVİDEBLOCK BÜLLÜK 2 \n");
        while (cur_ptr->limit != max)
        { 
        	cur = cur + sizeof(struct block);
        	cur_ptr = cur;
        }
        
        void * tmp = cur;
	struct block * tmp_ptr = (struct block *)tmp;
	
	printf("DİVİDEBLOCK BÜLLÜK 3 \n");
        
        //Tmp_ptr is the last node
        while(tmp_ptr->next != -1){
        	tmp = tmp + sizeof(struct block);
        	tmp_ptr = tmp;
        }
        tmp = tmp + sizeof(struct block);
        tmp_ptr = tmp;
        
        printf("DİVİDEBLOCK BÜLLÜK 4 \n");

        struct  block * new_tmp = tmp_ptr;
        
        new_tmp->next = -1;
        new_tmp->location = tmp_ptr;
        tmp += sizeof(struct block);
        tmp_ptr = tmp;
        
        printf("DİVİDEBLOCK BÜLLÜK 5 \n");

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
        
        printf("DİVİDEBLOCK BÜLLÜK 6 \n");
        
        int tmp_location = ((struct block *)cur)->location;
        int tmp_limit = ((struct block *)cur)->limit;
        int tmp_next = ((struct block *)cur)-> next;

    //Set the size of the created nodes.
    ((struct block *) tmp_ptr) -> limit = tmp_limit / 2;
    ((struct block *) tmp_ptr_next) -> limit = tmp_limit / 2;
    printf("DİVİDEBLOCK BÜLLÜK 7 \n");
    //Lower location variable returned.
    
    printList();
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
	
	//printList();
	sem_wait(&mutex);

	printf("FREE BÜLLÜK 1 \n");

	struct block * page_ptr = (struct block *)page_addr;

	if(page_ptr->next == -1){
		printf("U can not alloc before open in shared memory.");
		sem_post(&mutex);
		return;
	}   

	printf("FREE BÜLLÜK BEKLE BAKİİM \n");
	//Linkedlist pointer
	void * voidCurPtr = page_addr + sizeof(struct block);
	struct  block * current_ptr = (struct block *) voidCurPtr;

	printf("FREE BÜLLÜK 2 \n");

	//Pointer location
	int val = nextPower((int) (((SEG_SIZE / 256) / 2) + 3) * sizeof(struct block));

	//Ptr
	void * blockTmpPtr = page_addr + val;
	struct block * tmp = (struct block *) blockTmpPtr;

	printf("FREE BÜLLÜK 3 \n");

	//freelenmesi gereken mem location
	void * blockCurPtr = blockTmpPtr+ sizeof(struct block);
	struct block * cur = (struct block *) blockCurPtr;

	printf("FREE BÜLLÜK 4 \n");

	int mode = 1;

	if(current_ptr->location == val){
		mode = 0;
	}
	
	while(cur != ptr){
		//printf("FREE WHİLE BÜLLÜK 1 \n");

		if( mode == 0){
			printf("FREE İF BÜLLÜK 1 \n");

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

			//printf("FREE ELSE BÜLLÜK 1 \n");            
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

	//node yoksa
	if(current_ptr->next == -2){
		printf("FREE NODE YOK BÜLLÜK 1 \n");
		struct block * newNode;
		newNode->limit = ((struct block * )ptr)->limit;
		newNode->location =  ((struct block * )ptr)->location;
		newNode->next = -1;
		return ;
	}


	// tek node varsa
	//  current pointer sonrasına right shift
	// compaction.
	else if (current_ptr->next == -1)
	{
		printf("FREE TEK NODE BÜLLÜK 1 \n");
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
	// Ortasındaysa 
	// current pointer.next freelediğin yeri koy.

	else{
		printf("FREE ORTA BÜLLÜK 1 \n");
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
        
        
       



///**********************************************************

       // struct  block * location_next_ptr;


//        while(current_ptr->limit < (((struct block *) ptr)->limit)){
//            current_ptr = current_ptr + sizeof(struct block);
//       }
        /*
        location_next_ptr =  ((struct  block *) current_ptr);
        current_ptr = current_ptr - sizeof(struct block);    
     
        current_ptr->next = ((struct block *) ptr)->location;
        ((struct block *) ptr)->location = current_ptr + ()
        ((struct block *) ptr)-> next = location_next_ptr->location;
       
       
       struct block * end_node = current_ptr;
       while (end_node->next != -1)
       {
           end_node = end_node + sizeof(struct block);
       }
       struct block * newNode;
       //end_node = end_node + sizeof(struct block);
     
       while(end_node->location != current_ptr->location){
        
        newNode->location = end_node->location;
        newNode->limit = end_node->limit;
        newNode->next = end_node->next;
        newNode = end_node;
        end_node->location = end_node->location - sizeof(block struct);
       }
        current_ptr = current_ptr + sizeof(block struct);
        current_ptr->limit = ((struct  block *) ptr) -> limit;
        current_ptr->location = ((struct  block *)end_node) -> next;
        
        end_node =  end_node + (2 * sizeof(block));        
        current_ptr->next = ((struct  block *)end_node)->location;
*/
       /*      
        deleted_target->next = tmp->next;
        tmp->next = deleted_target;
        struct block * block_to_be_combined = deleted_target;     
        while(block_to_be_combined->limit == block_to_be_combined->next->limit)
        {
            block_to_be_combined = combineBlocks(block_to_be_combined,block_to_be_combined->next);
        }
        sem_post(&mutex);
        */
       
}



void combineBlocks(){
	
	bool go_through_list_again = false;

	do{
		struct block * cur = (struct block *) page_addr;
		cur = cur + sizeof(struct block);
	
		struct block * cur_next = cur + sizeof(struct block);
		go_through_list_again = false;

		//Traverse through whole list; if flag = 1, no compaction needed; otherwise, traverse and check again
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




	} while(go_through_list_again != false); //
    
}



int sbmem_close (){
    
    int t = munmap(page_addr, SEG_SIZE);
    p_map->next--;
    printf("To use the library again first call sbmem_open()");
   
}
