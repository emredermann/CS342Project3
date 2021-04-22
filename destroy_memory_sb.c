#include <unistd.h>
#include <stdlib.h>
#include "sbmem.h"
 

int main()
{
    sbmem_remove();
    sbmem_close();
    return 0;
}