#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>
#include "libcma.h"
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include <math.h>


#define DMA_ADDRESS 0x40400000
#define IP_ADDRESS 0x43C00000

#define MM2S_CONTROL_REGISTER 0x00
#define MM2S_STATUS_REGISTER 0x04
#define MM2S_START_ADDRESS 0x18
#define MM2S_LENGTH 0x28

#define S2MM_CONTROL_REGISTER 0x30
#define S2MM_STATUS_REGISTER 0x34
#define S2MM_DESTINATION_ADDRESS 0x48
#define S2MM_LENGTH 0x58

#define IP_CNTRL_REG 0x10
#define IP_I_ROW_REG 0x20
#define IP_W_ROW_REG 0x28
#define IP_W_COL_REG 0x30

#define IP_SEND_WEIGHTS 1 
#define IP_SEND_DATA 0

static volatile unsigned int* dma_virtual_address;
static volatile unsigned int* ip_virtual_address;
static int dh;

unsigned int ip_set(int offset, unsigned int value) {
    ip_virtual_address[offset>>2] = value;
}

unsigned int ip_get(int offset) {
    return ip_virtual_address[offset>>2];
}

unsigned int dma_set(int offset, unsigned int value) {
    dma_virtual_address[offset>>2] = value;
}

unsigned int dma_get(int offset) {
    return dma_virtual_address[offset>>2];
}

void dma_s2mm_status() {
    unsigned int status = dma_get(S2MM_STATUS_REGISTER);
    printf("Stream to memory-mapped status (0x%08x@0x%02x):", status, S2MM_STATUS_REGISTER);
    if (status & 0x00000001) printf(" halted"); else printf(" running");
    if (status & 0x00000002) printf(" idle");
    if (status & 0x00000008) printf(" SGIncld");
    if (status & 0x00000010) printf(" DMAIntErr");
    if (status & 0x00000020) printf(" DMASlvErr");
    if (status & 0x00000040) printf(" DMADecErr");
    if (status & 0x00000100) printf(" SGIntErr");
    if (status & 0x00000200) printf(" SGSlvErr");
    if (status & 0x00000400) printf(" SGDecErr");
    if (status & 0x00001000) printf(" IOC_Irq");
    if (status & 0x00002000) printf(" Dly_Irq");
    if (status & 0x00004000) printf(" Err_Irq");
    printf("\n");
}

void dma_mm2s_status() {
    unsigned int status = dma_get(MM2S_STATUS_REGISTER);
    printf("Memory-mapped to stream status (0x%08x@0x%02x):", status, MM2S_STATUS_REGISTER);
    if (status & 0x00000001) printf(" halted"); else printf(" running");
    if (status & 0x00000002) printf(" idle");
    if (status & 0x00000008) printf(" SGIncld");
    if (status & 0x00000010) printf(" DMAIntErr");
    if (status & 0x00000020) printf(" DMASlvErr");
    if (status & 0x00000040) printf(" DMADecErr");
    if (status & 0x00000100) printf(" SGIntErr");
    if (status & 0x00000200) printf(" SGSlvErr");
    if (status & 0x00000400) printf(" SGDecErr");
    if (status & 0x00001000) printf(" IOC_Irq");
    if (status & 0x00002000) printf(" Dly_Irq");
    if (status & 0x00004000) printf(" Err_Irq");
    printf("\n");
}

int dma_mm2s_sync() {
    unsigned int mm2s_status =  dma_get(MM2S_STATUS_REGISTER);
    while(!(mm2s_status & 1<<12) || !(mm2s_status & 1<<1) ){
        dma_s2mm_status();
        dma_mm2s_status();

        mm2s_status =  dma_get(MM2S_STATUS_REGISTER);
    }
}

int dma_s2mm_sync() {
    unsigned int s2mm_status = dma_get(S2MM_STATUS_REGISTER);
    while(!(s2mm_status & 1<<12) || !(s2mm_status & 1<<1)){
        dma_s2mm_status();
        dma_mm2s_status();

        s2mm_status = dma_get(S2MM_STATUS_REGISTER);
    }
}

void memdump_uint32(void* virtual_address, int data_length) {
    unsigned int *p = virtual_address;
    int offset;
    for (offset = 0; offset < data_length; offset++) {
        printf("index: %d value: %u\n", offset, p[offset]);
    }
}

void memset_uint32(void* virtual_address, int val, int num_count)
{
    unsigned int *p = virtual_address;
    int offset;
    for (offset = 0; offset < num_count; offset++) {
        p[offset] = val;
    }   
}

void dma_start(){
    printf("Starting S2MM channel with all interrupts masked...\n");
    dma_set(S2MM_CONTROL_REGISTER, 0x0001);
    dma_s2mm_status();

    printf("Starting MM2S channel with all interrupts masked...\n");
    dma_set(MM2S_CONTROL_REGISTER, 0x0001);
    dma_mm2s_status();
}

void dma_stop(){
    printf("Halting DMA\n");
    dma_set(S2MM_CONTROL_REGISTER, 0);
    dma_set(MM2S_CONTROL_REGISTER, 0);
    dma_s2mm_status();
    dma_mm2s_status();
}

void dma_reset(){
    printf("Resetting DMA\n");
    dma_set(S2MM_CONTROL_REGISTER, 4);
    dma_set(MM2S_CONTROL_REGISTER, 4);
    dma_s2mm_status();
    dma_mm2s_status();
}

void ip_start(){
    printf("Starting the ip\n");
    ip_virtual_address[0x00] = 0x81;
}

void ip_stop(){
    printf("Stopping the ip\n");
    ip_virtual_address[0x00] = 0x00;
}

void init_ip_params_size(unsigned int control, unsigned int w_col, unsigned int i_row){
    printf("Setting ip transfer size\n");

    ip_set(IP_CNTRL_REG,control);
    ip_set(IP_W_COL_REG, w_col);
    ip_set(IP_I_ROW_REG,i_row);

    // printf("IP_CNTRL_REG: %u\n", ip_get(IP_CNTRL_REG));
    // printf("IP_W_COL_REG: %u\n", ip_get(IP_CNTRL_REG));
    // printf("IP_I_ROW_REG: %u\n", ip_get(IP_CNTRL_REG));
}

void ip_params(unsigned int control, unsigned int w_col, unsigned int i_row){
    init_ip_params_size(control,w_col,i_row);
}

void dma_transaction(float* src, float* dst, unsigned int src_data_count, unsigned int dst_data_count){

    printf("Writing source address...\n");
    unsigned int src_phy = cma_get_phy_addr((void*)src);

    dma_set(MM2S_START_ADDRESS, src_phy); // Write source address
    dma_mm2s_status();

    unsigned int dst_phy = cma_get_phy_addr((void*)dst);

    printf("Writing destination address\n");
    dma_set(S2MM_DESTINATION_ADDRESS, dst_phy); // Write destination address
    dma_s2mm_status();

    ip_virtual_address[IP_CNTRL_REG>>2] = 0;

    printf("Writing S2MM transfer length...\n");
    dma_set(S2MM_LENGTH, dst_data_count);
    dma_s2mm_status();

    printf("Writing MM2S transfer length...\n");
    dma_set(MM2S_LENGTH, src_data_count);
    dma_mm2s_status();

    printf("Waiting for MM2S synchronization...\n");
    dma_mm2s_sync();

    printf("Waiting for S2MM sychronization...\n");
    dma_s2mm_sync(); 
}

void init_dma_and_ip_virtual_address(){
    printf("Initializing DMA and IP\n");
    dh = open("/dev/mem", O_RDWR | O_SYNC); // Open /dev/mem which represents the whole physical memory
    dma_virtual_address = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, dh, DMA_ADDRESS); // Memory map AXI Lite register block
    ip_virtual_address = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, dh, IP_ADDRESS); // Memory map AXI Lite register block
}

void reset_fabric(){

    printf("resetting fabric\n");

    int pid = fork();
    if (!pid)
    {
        char *new_argv[3];
        new_argv[0]="sudo";
        new_argv[1]="/usr/bin/python3";
        new_argv[2]="reset_fabric.py";
        new_argv[3]=0;
        execvp(new_argv[0],new_argv);
        perror("Python execution");
    }
    
    waitpid(pid, NULL, 0);
}

void dma_cma_test()
{

    // reset_fabric();

    // init_dma_and_ip();

    // unsigned int* src_buff  = (unsigned int*)cma_alloc(1024<<2, 0);

    // if(src_buff == NULL)
    // {
    //     printf("failed to allocated source buffer\n");
    //     return -1;
    // }

    // unsigned int* dst_buf = (unsigned int*)cma_alloc(1024<<2, 0);

    // if(dst_buf == NULL)
    // {
    //     printf("failed to allocate destination buffer\n");
    //     return -1;
    // }

    // memset_uint32(src_buff, 20, 512); // set source block
    // memset_uint32(dst_buf, 0, 1024); // Clear destination block

    // printf("Source memory block:      "); memdump_uint32(src_buff, 1024);
    // printf("Destination memory block: "); memdump_uint32(dst_buf, 1024);

    // ip_start();
    // dma_reset();
    // dma_stop();
    // dma_start();

    // // init_ip_transfer_size(1024);

    // dma_transaction(src_buff,dst_buf, 1024<<2, 1024<<2, 512);

    // printf("Destination memory block: "); memdump_uint32(dst_buf, data_length);

    // memset_uint32(dst_buf, 0, 1024); 

    // memset_uint32(src_buff, 0, 1024); 

    // int i;
    // for(i = 0; i<512; i++)
    // {
    //     src_buff[i] = 5;
    // }
    // for(i = 512; i<512+256; i++)
    // {
    //     src_buff[i] = 10;
    // }
    //     for(i = 512+256; i<1024; i++)
    // {
    //     src_buff[i] = 15;
    // }

    // dma_transaction(src_buff,dst_buf,512<<2,512<<2,256);
    // dma_transaction(src_buff+512,dst_buf+512,256<<2,256<<2,256);
    // dma_transaction(src_buff+512+256,dst_buf+512+256,256<<2,256<<2,1024);

    // printf("Destination memory block: "); memdump_uint32(dst_buf, data_length);

    // memset_uint32(src_buff, 0, 1024); 
    // dma_transaction(src_buff,dst_buf, 1024<<2, 1024<<2, 1024);

    // printf("Destination memory block: "); memdump_uint32(dst_buf, data_length);

    // cma_free(src_buff);
    // cma_free(dst_buf);
}

// void gemm_nn(int M, int N, int K, float ALPHA,
//     float *A, int lda,
//     float *B, int ldb,
//     float *C, int ldc)
// {
//     int i, j, t;
//     float* weight_matrix = A;
//     float* image_matrix = B;

//     static int conv_num = 0;

//     for(t = 0; t<M; t++){ //filter select



//         for (i = 0; i < N; ++i) { //Image_matrix row select
//             register float temp = 0;
//             for (j = 0; j < K; ++j){ //Weight_matrix row select/ Image matrix_col Select
//                 temp += ALPHA*weight_matrix[t*lda+j]*image_matrix[i*ldb+j];
//             }
//             C[t*ldc+i] = temp; 
//         }        
//     }
// }

int main() {

    int t,i,j;

    srand((unsigned int)time(NULL));

    reset_fabric();

    init_dma_and_ip_virtual_address();

    unsigned int w_col = 144;
    unsigned int i_row = 150;

    float* weight_buff  = (float*)cma_alloc(w_col<<2, 0);
    float* image_buff  = (float*)cma_alloc((i_row*w_col)<<2, 0);
    float* dst_buf = (float*)cma_alloc(i_row<<2, 0);

    for(j = 0; j<w_col; j++)
    {
        weight_buff[j] = 1;
    }

    for(j = 0; j<i_row*w_col; j++)
    {
        image_buff[j] = 2.5;
    }

    if(weight_buff == NULL || dst_buf == NULL)
    {
        printf("failed to allocated a buffer\n");
        return -1;
    }

    ip_virtual_address[IP_CNTRL_REG>>2] = 1;
    ip_virtual_address[IP_I_ROW_REG>>2] = 150;
    ip_virtual_address[IP_W_ROW_REG>>2] = 1;
    ip_virtual_address[IP_W_COL_REG>>2] = 144;

    ip_start();
    dma_reset();
    dma_stop();
    dma_start();

    // init_ip_params_size(IP_SEND_WEIGHTS, w_col, i_row);

    dma_transaction(weight_buff, dst_buf, w_col << 2, i_row << 2);

    for(i = 0; i<150; i++)
    {
        printf("%f\n", dst_buf[i]);
    }

    dma_transaction(image_buff, dst_buf, (w_col*i_row) << 2, i_row << 2);

    for(i = 0; i<150; i++)
    {
        printf("%f\n", dst_buf[i]);
    }

    cma_free(weight_buff);
    cma_free(image_buff);

}



