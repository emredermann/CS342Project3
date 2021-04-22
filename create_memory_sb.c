#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "sbmemlib.c"
 

int main(int argc, char *argv[])
{
    int segment_size;
    char * input;
    if (argc > 1) {
        segment_size = atoi(argv[1]);  
    }
    sbmem_init(segment_size);
    sbmem_open();
    return 0;
}