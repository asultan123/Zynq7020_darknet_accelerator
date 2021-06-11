#ifndef GEMM_H
#define GEMM_H
#include "activations.h"
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

void convolution_2d(int w, int h, int ksize, int n, int c, int pad, int stride,
    float *weights, float *input, float *output, float *mean);

static inline void set_bit(unsigned char *const dst, size_t index) {
    size_t dst_i = index / 8;
    int dst_shift = index % 8;
    dst[dst_i] |= 1 << dst_shift;
    //dst[dst_i] |= 1 << (8 - dst_shift);
}

static inline unsigned char get_bit(unsigned char const*const src, size_t index) {
    size_t src_i = index / 8;
    int src_shift = index % 8;
    unsigned char val = (src[src_i] & (1 << src_shift)) > 0;
    //unsigned char val = (src[src_i] & (1 << (8 - src_shift))) > 0;
    return val;
}

int is_avx();
int is_fma_avx2();

void float_to_bit(float *src, unsigned char *dst, size_t size);

void transpose_block_SSE4x4(float *A, float *B, const int n, const int m,
    const int lda, const int ldb, const int block_size);

void transpose_bin(uint32_t *A, uint32_t *B, const int n, const int m,
    const int lda, const int ldb, const int block_size);

void gemm_nn_custom_bin_mean_transposed(int M, int N, int K, float ALPHA_UNUSED,
    unsigned char *A, int lda,
    unsigned char *B, int ldb,
    float *C, int ldc, float *mean_arr);

void im2col_cpu_custom(float* data_im,
    int channels, int height, int width,
    int ksize, int stride, int pad, float* data_col);

void im2col_cpu_custom_align(float* data_im,
    int channels, int height, int width,
    int ksize, int stride, int pad, float* data_col, int bit_align);

void im2col_cpu_custom_bin(float* data_im,
    int channels, int height, int width,
    int ksize, int stride, int pad, float* data_col, int bit_align);

void im2col_cpu_custom_transpose(float* data_im,
    int channels, int height, int width,
    int ksize, int stride, int pad, float* data_col, int ldb_align);

void activate_array_cpu_custom(float *x, const int n, const ACTIVATION a);

void transpose_32x32_bits_reversed_diagonale(uint32_t *A, uint32_t *B, int m, int n);

void gemm_bin(int M, int N, int K, float ALPHA,
        char  *A, int lda,
        float *B, int ldb,
        float *C, int ldc);

void repack_input(float *input, float *re_packed_input, int w, int h, int c);

void convolution_repacked(uint32_t *packed_input, uint32_t *packed_weights, float *output,
    int w, int h, int c, int n, int size, int pad, int new_lda, float *mean_arr);

void gemm_nn_bin_32bit_packed(int M, int N, int K, float ALPHA,
    uint32_t *A, int lda,
    uint32_t *B, int ldb,
    float *C, int ldc, float *mean_arr);

void transpose_uint32(uint32_t *src, uint32_t *dst, int src_h, int src_w, int src_align, int dst_align);

void gemm_nn_bin_transposed_32bit_packed(int M, int N, int K, float ALPHA,
    uint32_t *A, int lda,
    uint32_t *B, int ldb,
    float *C, int ldc, float *mean_arr);


void forward_maxpool_layer_avx(float *src, float *dst, int *indexes, int size, int w, int h, int out_w, int out_h, int c,
    int pad, int stride, int batch);


void gemm(int TA, int TB, int M, int N, int K, float ALPHA,
                    float *A, int lda,
                    float *B, int ldb,
                    float BETA,
                    float *C, int ldc);

void gemm_cpu(int TA, int TB, int M, int N, int K, float ALPHA,
        float *A, int lda,
        float *B, int ldb,
        float BETA,
        float *C, int ldc);

#ifdef GPU
void gemm_ongpu(int TA, int TB, int M, int N, int K, float ALPHA,
        float *A_gpu, int lda,
        float *B_gpu, int ldb,
        float BETA,
        float *C_gpu, int ldc);

void gemm_gpu(int TA, int TB, int M, int N, int K, float ALPHA,
        float *A, int lda,
        float *B, int ldb,
        float BETA,
        float *C, int ldc);

// ****************************************DRIVER STUFF*******************************************
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


#define layer_k_1 27
#define layer_k_2 144
#define layer_k_3 32
#define layer_k_4 144
#define layer_k_5 128
#define layer_k_6 144
#define layer_k_7 128
#define layer_k_8 288
#define layer_k_9 256
#define layer_k_10 288
#define layer_k_11 256
#define layer_k_12 576
#define layer_k_13 512
#define layer_k_14 576
#define layer_k_15 512
#define layer_k_16 128 
#define layer_k_17 0


volatile unsigned int* dma_virtual_address;
volatile unsigned int* ip_virtual_address;
volatile int dh;

unsigned int ip_set(int offset, unsigned int value;

unsigned int dma_set(int offset, unsigned int value);

unsigned int dma_get(int offset);

void dma_s2mm_status();

void dma_mm2s_status();

int dma_mm2s_sync();

int dma_s2mm_sync();

void memdump_uint32(void* virtual_address, int data_length);

void memset_uint32(void* virtual_address, int val, int num_count);

void dma_start();

void dma_stop();

void dma_reset();

void ip_start();

void ip_stop();

void init_ip_params_size(unsigned int control, unsigned int w_col, unsigned int i_row);

void ip_params(unsigned int control, unsigned int w_col, unsigned int i_row);

void dma_transaction(unsigned int src, unsigned int dst, unsigned int src_data_count, \
    unsigned int dst_data_count, unsigned int control, unsigned int w_col, unsigned int i_row);


void init_dma_and_ip_virtual_address();

void conv_layer_1(float* A, float* B, float* C);

void conv_layer_2(float* A, float* B, float* C);

void conv_layer_3(float* A, float* B, float* C);

void conv_layer_4(float* A, float* B, float* C);

void conv_layer_5(float* A, float* B, float* C);

void conv_layer_6(float* A, float* B, float* C);

void conv_layer_7(float* A, float* B, float* C);

void conv_layer_8(float* A, float* B, float* C);

void conv_layer_9(float* A, float* B, float* C);

void conv_layer_10(float* A, float* B, float* C);

void conv_layer_11(float* A, float* B, float* C);

void conv_layer_12(float* A, float* B, float* C);

void conv_layer_13(float* A, float* B, float* C);

void conv_layer_14(float* A, float* B, float* C);

void conv_layer_15(float* A, float* B, float* C);


// *********************************** END DRIVER STUFF*******************************************


void print_array(float* arr, int count, char* name);

#endif
#ifdef __cplusplus
}
#endif
#endif
