#ifndef SC_DISABLE_API_VERSION_CHECK
#define SC_DISABLE_API_VERSION_CHECK
#endif

#include <systemc.h>
#include <cynw_p2p.h>

#include "batcher_odd_even_merge_sort_input_type.h"
#include "batcher_odd_even_merge_sort_output_type.h"

SC_MODULE(batcher_odd_even_merge_sort)
{
    sc_in_clk    clk;
    sc_in<bool>  rst;

    cynw_p2p < batcher_odd_even_merge_sort_INPUT_DT  >::in   inputs;
    cynw_p2p < batcher_odd_even_merge_sort_OUTPUT_DT >::out  outputs;

    SC_CTOR(batcher_odd_even_merge_sort)
    {
        SC_CTHREAD(sort_process, clk.pos());
        reset_signal_is(rst, false);   // active-LOW reset (matches tb: rst=0 during reset)
    }

    void sort_process()
    {
        // Reset state — initialise all channel outputs
        inputs.reset();
        outputs.reset();
        wait();

        while (true)
        {
            // Block until a transaction arrives
            batcher_odd_even_merge_sort_INPUT_DT in_pkt = inputs.get();

            sc_int<16> s[8];
            for (int i = 0; i < 8; i++)
                s[i] = in_pkt.data[i];

            // Stage 1 — sort adjacent pairs
            wait();
            { sc_int<16> a=s[0],b=s[1]; if(a>b){s[0]=b;s[1]=a;} }
            { sc_int<16> a=s[2],b=s[3]; if(a>b){s[2]=b;s[3]=a;} }
            { sc_int<16> a=s[4],b=s[5]; if(a>b){s[4]=b;s[5]=a;} }
            { sc_int<16> a=s[6],b=s[7]; if(a>b){s[6]=b;s[7]=a;} }

            // Stage 2 — sort stride-2 pairs
            wait();
            { sc_int<16> a=s[0],b=s[2]; if(a>b){s[0]=b;s[2]=a;} }
            { sc_int<16> a=s[1],b=s[3]; if(a>b){s[1]=b;s[3]=a;} }
            { sc_int<16> a=s[4],b=s[6]; if(a>b){s[4]=b;s[6]=a;} }
            { sc_int<16> a=s[5],b=s[7]; if(a>b){s[5]=b;s[7]=a;} }

            // Stage 3 — finish independent half-sorts
            wait();
            { sc_int<16> a=s[1],b=s[2]; if(a>b){s[1]=b;s[2]=a;} }
            { sc_int<16> a=s[5],b=s[6]; if(a>b){s[5]=b;s[6]=a;} }

            // Stage 4 — cross-half merge first pass
            wait();
            { sc_int<16> a=s[0],b=s[4]; if(a>b){s[0]=b;s[4]=a;} }
            { sc_int<16> a=s[1],b=s[5]; if(a>b){s[1]=b;s[5]=a;} }
            { sc_int<16> a=s[2],b=s[6]; if(a>b){s[2]=b;s[6]=a;} }
            { sc_int<16> a=s[3],b=s[7]; if(a>b){s[3]=b;s[7]=a;} }

            // Stage 5 — sub-merge inner pairs
            wait();
            { sc_int<16> a=s[2],b=s[4]; if(a>b){s[2]=b;s[4]=a;} }
            { sc_int<16> a=s[3],b=s[5]; if(a>b){s[3]=b;s[5]=a;} }

            // Stage 6 — final interleave
            wait();
            { sc_int<16> a=s[1],b=s[2]; if(a>b){s[1]=b;s[2]=a;} }
            { sc_int<16> a=s[3],b=s[4]; if(a>b){s[3]=b;s[4]=a;} }
            { sc_int<16> a=s[5],b=s[6]; if(a>b){s[5]=b;s[6]=a;} }

            // Write sorted result
            batcher_odd_even_merge_sort_OUTPUT_DT out_pkt;
            for (int i = 0; i < 8; i++)
                out_pkt.data[i] = s[i];

            outputs.put(out_pkt);
        }
    }
};