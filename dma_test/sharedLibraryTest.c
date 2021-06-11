#include <stdint.h> 
#include <stdio.h>
#include "libcma.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int cma_test(){

 	clock_t start, end;
 	double cpu_time_used;

	int i;
	int size = 1024;
	int length = size*4;
	float page_size = 4096;

	printf("Done!\n");

	printf("memory available: %f\n", cma_pages_available()*page_size/1024/1024);

	printf("attempting to allocate buffer\n");
	
	int* buf = (int*)cma_alloc(length,0);
	
	if(buf==NULL)
	{
		printf("failed to allocate buffer\nexiting...\n");
		return -1;
	}

	printf("prepping src\n");

	int* src = (int*)malloc(length);

	if(src==NULL){
		printf("failed to allocate src\nexiting...\n");
		return -1;
	}

	memset(src,10,length);

	unsigned long phy_addr = cma_get_phy_addr((void*)buf);

	printf("allocated buffer at physical address: %lu\n",phy_addr);

	printf("writing to buffer and validating\n");
    
    start = clock();

	memcpy(buf,src,length);

	int res = memcmp(buf,src,length);

	if(res!=0)
	{
		printf("mismatch between buffers\n");
	}

 	end = clock();

	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

	printf("time taken: %f\n", cpu_time_used);

	printf("De-allocating buffer\n");
	cma_free((void*)buf);
	free(src);
	printf("Buffer de-allocated... exiting\n");

} 

static volatile uint32_t *dma_base_reg;


void mmio_dma_test(){
	    
	    int fd ;

        //Obtain handle to physical memory
        if ((fd = open ("/dev/mem", O_RDWR | O_SYNC) ) < 0) {
                printf("Unable to open /dev/mem: %s\n", strerror(errno));
                return -1;
        }

        dma_base_reg = (uint32_t *)mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x20200000);

        if ((int32_t)gpio < 0){
                printf("Mmap failed: %s\n", strerror(errno));
                return -1;
        }
}

int main(int argc, char const *argv[])
{
 	
	return cma_test();	
}