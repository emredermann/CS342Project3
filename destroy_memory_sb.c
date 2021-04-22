#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "sbmem.h"
 

int main(int argc, char *argv[])
{
    printf("Destroy başlıyoruz");
 
    
    sbmem_remove();
    
    
    printf("Destroy kapatıyoruz");
    return 0;
}