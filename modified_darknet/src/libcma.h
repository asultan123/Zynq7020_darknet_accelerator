#ifndef LIBCMA_H_   
#define LIBCMA_H_
#include <stdint.h> 

static uint32_t xlnkBufCnt = 0;
unsigned long cma_mmap(uint32_t phyAddr, uint32_t len);
unsigned long cma_munmap(void *buf, uint32_t len);
void *cma_alloc(uint32_t len, uint32_t cacheable);
unsigned long cma_get_phy_addr(void *buf);
void cma_free(void *buf);
uint32_t cma_pages_available();
void cma_flush_cache(void* buf, unsigned int phys_addr, int size);
void cma_invalidate_cache(void* buf, unsigned int phys_addr, int size);
void _xlnk_reset();

#endif 


