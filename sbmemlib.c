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

void * page_addr;
struct block * p_map;
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
    p_map->next = -1;
   
    linkedlistInit();
    printList();
    return 0;
    
}


void linkedlistInit(){

    int i  = 7;
    void * ptr = page_addr;
    struct block * head = page_addr + sizeof(struct block);

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

struct block* DivideBlock( int realsize,int max){
    
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
        //Linkedlist pointer
    	struct  block * current_ptr = page_addr + sizeof(struct block);
        //Pointer location
        int val = (int) (((segsize / 256) / 2) + 3) * sizeof(struct block);
        //Ptr
        struct  block * tmp = page_addr + val;
        //freelenmesi gereken mem location
        struct block * cur = tmp + sizeof(block);
        
        /*
            Search part
        */

       int mode = 1;
       if(cur->location == val){
           mode = 0;
           while(cur != ptr){
               if( mode == 0){
                   if((cur->location + cur->limit) == cur->next) {
                       val = val + cur->limit;
                       cur = cur + sizeof(struct block);
                   }else{
                       val = val + cur->limit;
                       tmp = (struct block)(page_addr + val);
                       mode = 1;

                       // 2 * sizeof(struct block) ne demek??
                       cur = tmp +2*sizeof(struct block);
                   }
               }
               else{
                   if((val + (tmp->limit)) == cur->next || (val+(tmp->limit)) == cur->location){
                       val = val + tmp->limit;
                       mode = 0;
                       cur = cur + sizeof(struct block);
                   }else {
                       val = val + tmp->limit;
                       tmp = (struct * block) page_addr + val;
                       // 2* sizeofssadsad
                       cur = tmp+2*sizeof(struct block);
                   }
               }
               //unable to find
               if(cur->next == -1){
                   printf("Could not founded");
     //              sem_post(&mutex);
                   return
               }
           }
       }
///**********************************************************

        struct  block * location_next_ptr;

    	if(pid == -1){
            printf("U can not alloc before open in shared memory.");
            return;
        }   
//        while(current_ptr->limit < (((struct block *) ptr)->limit)){
//            current_ptr = current_ptr + sizeof(struct block);
//       }
        
        location_next_ptr =  ((struct  block *) current_ptr);
        current_ptr = current_ptr - sizeof(struct block);    
       /*
        current_ptr->next = ((struct block *) ptr)->location;
        ((struct block *) ptr)->location = current_ptr + ()
        ((struct block *) ptr)-> next = location_next_ptr->location;
        */
       
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



// Sağdakini ()locationu büyük olanı küçük olanın içine koy(loc küçük olanın limitinini 2 katına çıkar ondan sonraki nodeları bir sola kaydır.)
struct block * combineBlocks(struct block * lower_location,struct block * higher_location)
{
	//sem_wait(&mutex);
	void * cur = page_addr;
    void * cur_next;
    ((struct block *) cur)->limit = ((struct block *) cur)->limit *2;
    struct block * result;
    result = cur; 
    cur = cur + lower_location->location;
    
    // Left shift operation
    while(((struct block *) cur)->next != -1){
        cur = cur + sizeof(struct block);
        cur_next = cur + sizeof(struct block);
        
        ((struct block *) cur)->limit = ((struct block *) cur_next)->limit;
        ((struct block *) cur)->location = ((struct block *) cur_next)->location;
        ((struct block *) cur)->next = ((struct block *) cur_next)->next;
    }
    return result;
}


int sbmem_close (){
    //No of process düşür
    int t = munmap(page_addr, SEG_SIZE);
    printf("To use the library again first call sbmem_open()");
   
}
