int sbmem_init (int segsize);


/*
use shm_unlink () function to implement this. The function
will do all the necessary cleanup.
*/
sbmem_remove ();


int sbmem_open();
void *sbmem_alloc (int reqsize);
void sbmem_free (void *ptr);
int sbmem_close ();