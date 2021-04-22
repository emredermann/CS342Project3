#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "sbmem.h"
 

int main(int argc, char *argv[])
{
    printf("Başlıyoruz");
    int segment_size;
    if (argc > 1) {
        segment_size = atoi(argv[1]);  
    }

    sbmem_init(segment_size);
     printf("Kapatıyoruz");
    return 0;
}