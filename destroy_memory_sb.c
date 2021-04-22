#include <unistd.h>
#include <stdlib.h>
#include "sbmemlib.c"
 

int main()
{
    sbmem_remove();
    sbmem_close();
    return 0;
}