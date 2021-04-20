#include <unistd.h>
#include <stdlib.h>
#include "sbmem.h"
 
#define SEGMENT_SIZE 256

int main()
{
    sbmem_init(SEGMENT_SIZE);
    return 0;
}