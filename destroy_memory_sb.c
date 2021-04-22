#include <unistd.h>
#include <stdlib.h>
#include "sbmem.h"
 

int main()
{
    printf("Destroy başlıyoruz");
    sbmem_remove();
    printf("Destroy kapatıyoruz");
    

    return 0;
}