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
    
     management_size = (int) (((segsize / 256) / 2) + 3) * sizeof(struct block);
    //Initializes the shared memory maps it to the p_map
    printf("Before mmap*************************************** \n");
    page_addr =  mmap( NULL, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    
    if(p_map == MAP_FAILED){
        printf( "Mmap failed !!: \n");
        return -1;
    } printf("after mmap*************************************** \n");

   

    p_map = (struct block *) page_addr;
    p_map->limit = SEG_SIZE;
    p_map->location = 0;
    // Active processor
    p_map->next = 0;
    list_head = p_map + sizeof(struct block);
   
    linkedlistInit();
    printList();
    return 0;
    
}


void linkedlistInit(){

    int i  = 7;
    void * ptr = page_addr;
    struct block * head = page_addr + sizeof(struct block);
    int count = 0;
    int init_segsize = management_size;
   while(init_segsize > management_size){
       init_segsize = init_segsize /2;
       count ++;
   }
   int tmpa = management_size;
   int tmpb = management_size * 2;
   for (int i = 0; i < count -1; i++){
       head->location = tmpa;
       head->limit = tmpa;
       head->next = tmpb;
   
    tmpa = tmpb;
    tmpb = tmpb *2;
    head = head + sizeof(struct block);
   }
    head->limit = tmpa;
    head->location = tmpa;
    head->next = -1;
   
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
    
     p_map->next++;
    page_addr = mmap(0, minimum_segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
 
    
    if(page_addr == MAP_FAILED){
       printf( "Mmap failed: \n");
       sem_post(&mutex);
       return -1;
    }
    struct block * tmp = (struct block *) page_addr;
    
    
    SEG_SIZE = tmp->limit;

    if(p_map->next > 10){
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
    
    

    printf("Linked list called safely. \n");

    pid = getpid();
    printf("pid %d\n", pid);
    p_map->next++;
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
        //tmp = tmp + sizeof(struct block);
        struct block * tmp_next;

        // Takes the result pointer 
        struct block * result = tmp; 
        tmp_next = page_addr + tmp->location;
      

      // Left shift operation
        while(tmp_next->location != -1){
            tmp->limit = tmp_next->limit;
            tmp->location = tmp_next->location;
            tmp->next = tmp_next->next;
            tmp = page_addr + sizeof(struct block);
            tmp_next = tmp_next + sizeof(struct block);
            }     
    
        tmp_next = tmp_next + sizeof(struct block);
        result->next = -2;
        return (void *) result;
    }
    

    // Tam fit yok ama gireceği alan var; gireceği alanın pointerını ver.
    return DivideBlock (realsize,max);













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
 
// To chehk the list
 printList();
 }


void printList(){
     struct  block * tmp = (struct  block *)page_addr;
     tmp = tmp + sizeof(struct  block);
     while(tmp->next != -1){
         printf("%d,%d,%d \n",tmp->location,tmp->limit,tmp->next);
     }
     printf("%d,%d,%d \n",tmp->location,tmp->limit,tmp->next);

}


// Realsize = requested size + sizeof(struct block)

struct block * DivideBlock( int realsize,int max){
    
	    void * cur = page_addr + sizeof(struct block);
        while (((struct block *)cur)->limit != max){ cur = cur + sizeof(struct block);}
              
        //Right shift
        void * tmp_ptr = cur;
        //Tmp_ptr is the last node
        while(tmp_ptr != -1){tmp_ptr = tmp_ptr + sizeof(struct block);}
        tmp_ptr = tmp_ptr + sizeof(struct block);

        struct  block * new_tmp = (struct block *) tmp_ptr;
        new_tmp->next = -1;
        new_tmp->location = tmp_ptr;
        tmp_ptr += sizeof(struct block);

        void * tmp_ptr_next;
        while ( tmp_ptr != cur)
        {   
            // Set the tmp_ptr_next as new created space for shifting initially.
            tmp_ptr_next = tmp_ptr + sizeof(struct block);
            
            // Shifted all the elements to the right starting from last node to cur. 
            ((struct block *)tmp_ptr_next)->limit = ((struct block *)tmp_ptr)->limit;
        
        // Opens location for the clonned struct 
            ((struct block *)tmp_ptr_next)->location = ((struct block *)tmp_ptr)->location + sizeof(struct block);
            ((struct block *)tmp_ptr_next)->next = ((struct block *)tmp_ptr)->next;
        // As a last copy process the cur node will be copied to two consecutive nodes
            tmp_ptr -= sizeof(struct block);
        }
        
        int tmp_location = ((struct block *)cur)->location;
        int tmp_limit = ((struct block *)cur)->limit;
        int tmp_next = ((struct block *)cur)-> next;

    //Set the size of the created nodes.
    ((struct block *) tmp_ptr) -> limit = tmp_limit / 2;
    ((struct block *) tmp_ptr_next) -> limit = tmp_limit / 2;
    
    //Lower location variable returned.
    return ((struct block *) tmp_ptr);


/*
        struct  block * new_block_1;
        struct  block * new_block_2;
        
        new_block_1->limit = tmp_limit / 2;
        new_block_2->limit = tmp_limit / 2;
        
        
        new_block_1->location = tmp_location;
        new_block_2->location = tmp_location + (tmp_limit / 2);


        cur -> next = new_block_1;
        new_block_1->next = new_block_2;
        new_block_2->next = tmp_next;

        return new_block_1;
 */    
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
    	sem_wait(&mutex);

    	if(p_map->next == -1){
            printf("U can not alloc before open in shared memory.");
            return;
        }   
        //Linkedlist pointer
    	struct  block * current_ptr = page_addr + sizeof(struct block);
        //Pointer location
        int val = nextPower((int) (((segsize / 256) / 2) + 3) * sizeof(struct block));
        //Ptr
        struct block * tmp = page_addr + val;
        //freelenmesi gereken mem location
        struct block * cur = tmp + sizeof(struct block);
        
       int mode = 1;

       if(current_ptr->location == val){
           mode = 0;
           while(cur != ptr){

               if( mode == 0){
                   
                   if(((current_ptr->location) + (current_ptr->limit)) == current_ptr->next) {
                       val = val + current_ptr->limit;
                       current_ptr = current_ptr + sizeof(struct block);
                   }else{
                       val = val + current_ptr->limit;
                       tmp = (struct block *)(page_addr + val);
                       mode = 1;
                       cur = tmp + sizeof(struct block);
                   }
               }
               else{                  
                   if((val + (tmp->limit)) == current_ptr->next || (val + (tmp->limit)) == current_ptr->location){
                        val = val + tmp->limit;
                        mode = 0;
                        current_ptr = current_ptr + sizeof(struct block);
                   }else {
                        val = val + tmp->limit;
                        tmp = (struct block * ) page_addr + val;
                        cur = tmp +  sizeof(struct block);
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
            struct block * shift_pointer = current_ptr;
            while(shift_pointer->next != -1){shift_pointer = shift_pointer +sizeof(struct block);}
            
            struct block * newNode = shift_pointer + sizeof(struct block);
            
            newNode->next = -1;
            newNode->limit = shift_pointer->limit;
            newNode->location = shift_pointer->next;

            while(shift_pointer->location != current_ptr->location){
                int nextLocation = shift_pointer->location;
                newNode = shift_pointer;
                shift_pointer = shift_pointer - sizeof(struct block);
                newNode->limit = shift_pointer->limit;
                newNode->location = shift_pointer->next;
                newNode->next = nextLocation;
            }
            shift_pointer = shift_pointer + sizeof(struct block);
            current_ptr->next = ((struct block * )ptr)->location;
            ((struct block * )ptr)->next = shift_pointer->location;
            combineBlocks();

        }
        
        
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


					cur->limit = cur>limit * 2;
					cur->next = cur_next->next;
					go_through_list_again = true; // Signal for repetition

					struct Block * nextN = cur_next + sizeof(struct Block);

					while(cur_next->next != -1){

						cur_next->location = nextN->location;
						cur_next->limit = nextN->limit;
						cur_next->next = nextN->next;
						nextN = nextN + sizeof(struct Block);
						cur_next = cur_next + sizeof(struct Block);
					}

					break;

				}
			}

		    	// Continue
		    	cur = cur + sizeof(struct Block);
		    	cur_next = cur_next + sizeof(struct Block);

		}




	} while(go_through_list_again != false); //
    
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

int sbmem_close (){
    
    int t = munmap(page_addr, SEG_SIZE);
    p_map->next--;
    printf("To use the library again first call sbmem_open()");
   
}
