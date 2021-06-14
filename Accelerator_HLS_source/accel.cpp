#include <stdio.h>
#include <stdlib.h>

#include "accel_simple.h"

///////////////////////////////////////////////////
t_rip rcv;

// --------------------------------------------------------------------
// function to be accelerated in HW wrapped with AXI4-Stream interface
//size 0 --> 72,  1--> 64
void accel_simple(AXI_VAL in_stream[IS_SIZE], AXI_VAL out_stream[OS_SIZE], bool control, bool mode, int i_row_sig, int w_row_sig, int w_col_sig)
{
#pragma HLS INTERFACE s_axilite port = mode bundle = CONTROL_BUS
    //mode 0--> fix weight stream image
//#pragma HLS INTERFACE s_axilite port=size bundle=CONTROL_BUS
#pragma HLS INTERFACE s_axilite port = control bundle = CONTROL_BUS
#pragma HLS INTERFACE s_axilite port = return bundle = CONTROL_BUS
#pragma HLS INTERFACE axis port = in_stream
#pragma HLS INTERFACE axis port = out_stream
    volatile ap_uint<1> last_test_cpp;
    bool my_flag = 0;
    // Union used for type conversion
    union
    {
        axi_T packet;
        struct
        {
            T f0;
            T f1;
        } val;
    } converter;

    int is_idx = 0;
    int os_idx = 0;
    static a16 tmp[i_1_row];
#pragma HLS ARRAY_PARTITION variable = tmp cyclic factor = 50 dim = 1
    static W16 A[w_col]; // weight
    W16 alpha;
    static f16 C[i_1_row][i_col];
#pragma HLS ARRAY_PARTITION variable = C cyclic factor = 50 dim = 1
    // second image

    if (control)
    {
    LOAD_A_2:
        for (int j = 0; j < w_col; j += DATA_PER_PACKET)
        {
            if (j < w_col_sig)
            {
                converter.packet = pop_stream(in_stream[is_idx++]);
                A[j] = (W16)converter.val.f0;
                A[j + 1] = (W16)converter.val.f1;
            }
            else
            {
                break;
            }
        }

        // Iterate over batch elements
    L1:
        for (int i = 0; i < i_1_row; i += DATA_PER_PACKET)
        {
            // Iterate over output classes
            converter.val.f0 = (T)5;
            converter.val.f1 = (T)5;
            //						out_stream[os_idx] = push_stream(converter.packet, 2*(os_idx == 2*((i_row_sig*w_row_sig)-1)));
            out_stream[os_idx] = push_stream(converter.packet, os_idx == 74);
            os_idx++;
        }
    }
    else
    {

    LOAD_C_1:
        for (int i = 0; i < i_1_row; i++)
        {
            tmp[i] = 0;
        LOAD_C_2:
            for (int j = 0; j < i_col; j += DATA_PER_PACKET)
            {
#pragma HLS PIPELINE
                if (j < w_col_sig)
                {
                    converter.packet = pop_stream(in_stream[is_idx++]);
                    C[i][j] = (f16)converter.val.f0;
                    C[i][j + 1] = (f16)converter.val.f1;
                }
                else
                {
                    break;
                }
            }
        }

    // Iterate over output classes
    L5:
        for (int j = 0; j < w_col; j++)
        {
            if (j >= w_col_sig)
            {
                break;
            }
            alpha = A[j];
        L6:
            for (int k = 0; k < i_1_row; k++)
            {
#pragma HLS PIPELINE
#pragma HLS UNROLL
                tmp[k] = tmp[k] + (a16)(alpha * C[k][j]);
            }
        }

    L7:
        for (int k = 0; k < i_1_row; k += DATA_PER_PACKET)
        {
            converter.val.f0 = (float)tmp[k];
            converter.val.f1 = (float)tmp[k + 1];
            out_stream[os_idx] = push_stream(converter.packet, os_idx == 74);
            os_idx++;
        }
    }
}

// --------------------------------------------------------
// functions to insert and extract elements from an axi stream
// includes conversion to correct data tyout_data_tpe
axi_T pop_stream(AXI_VAL &e)
{
#pragma HLS INLINE
    t_rip rcv;
    rcv.i = e.data;
    axi_T ret = rcv.f;

    volatile ap_uint<sizeof(axi_T)> strb = e.strb;
    volatile ap_uint<sizeof(axi_T)> keep = e.keep;
    volatile ap_uint<AXI_U> user = e.user;
    volatile ap_uint<1> last = e.last;
    volatile ap_uint<AXI_TI> id = e.id;
    volatile ap_uint<AXI_TD> dest = e.dest;

    return ret;
}

AXI_VAL push_stream(axi_T &v, bool last = false)
{
#pragma HLS INLINE

    AXI_VAL e;
    t_rip t;
    t.f = v;
    e.data = t.i;
    e.strb = (1 << sizeof(axi_T)) - 1;
    e.keep = (1 << sizeof(axi_T)) - 1;
    e.user = 0;
    e.last = last ? 1 : 0;
    e.id = 0;
    e.dest = 0;

    return e;
}
