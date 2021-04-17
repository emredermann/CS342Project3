
#ifndef SBMEM_H
#define SBMEM_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/mman.h>


/*
* Segsize is in bytes, must be power of 2
* Creation and initialization of a shared memory segment of the given size.
* Allocated in the requesting processes.
*/
int sbmem_init (int segsize);

/*
use shm_unlink () function to implement this. The function
will do all the necessary cleanup.
*/
bool sbmem_remove ();
int sbmem_open();
void *sbmem_alloc (int reqsize);
void sbmem_free (void *ptr);
int sbmem_close ();

#endif