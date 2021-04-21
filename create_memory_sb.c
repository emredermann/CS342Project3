#include <unistd.h>
#include <stdlib.h>
#include "sbmemlib.c"
 
#define SEGMENT_SIZE 256

int main()
{
    sbmem_init(SEGMENT_SIZE);
    return 0;
}