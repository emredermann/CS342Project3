include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "sbmem.h"
 

int main() //Argümanları kaldırdım
{
    printf("Destroy başlıyoruz\n");
 
    
    sbmem_remove();
    
    
    printf("Destroy kapatıyoruz\n");
    return 0;
}
