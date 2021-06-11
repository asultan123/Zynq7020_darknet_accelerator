#include <ap_axi_sdata.h>
#include <ap_fixed.h>

///////////////////////////////////////////////

///////////////////////

typedef unsigned long long axi_T;
typedef float T;
typedef ap_fixed<18, 8> f16; // for multiplication
typedef ap_fixed<18, 8> W16; // for weights
typedef ap_fixed<48, 10> a16; // for accumulation
#define DATA_PER_PACKET 2

////////////////////////////////////

typedef union
    {
          float f;
          unsigned int i;
    } t_rip;

// Input/Output Stream Size
#define w_row 1	// number of filter
#define w_col 576	// size of the filter
//#define w_col_2 128

#define i_1_row 150	// number of batches

#define i_col 576	// size of the filter

#define IS_SIZE i_1_row*w_col
#define OS_SIZE i_1_row*w_row




// AXI settings
#define AXI_DATA (sizeof(axi_T)*8)
#define AXI_U 4
#define AXI_TI 5
#define AXI_TD 5

typedef ap_axiu<AXI_DATA,AXI_U,AXI_TI,AXI_TD> AXI_VAL;

// Matrix Multiply prototype
void accel_simple (AXI_VAL in_stream[IS_SIZE],AXI_VAL out_stream[OS_SIZE],bool control,bool mode, int i_row_sig, int w_row_sig,int w_col_sig);

// AXI stream push and pop
axi_T pop_stream(AXI_VAL  &e);
AXI_VAL push_stream(axi_T  &v, bool last);

