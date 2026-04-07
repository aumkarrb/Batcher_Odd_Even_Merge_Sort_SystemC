#ifndef __TB_DRIVER__H
#define __TB_DRIVER__H

#include <systemc.h>
#include <esc.h>
#include <cynw_p2p.h>

#include "batcher_odd_even_merge_sort_input_type.h"
#include "batcher_odd_even_merge_sort_output_type.h"

SC_MODULE(tb_driver)
{
public:
    // Channels match system.h: DUT output -> tb input, DUT input -> tb output
    cynw_p2p < batcher_odd_even_merge_sort_OUTPUT_DT >::base_in    inputs;
    cynw_p2p < batcher_odd_even_merge_sort_INPUT_DT  >::base_out   outputs;

    sc_in_clk        clk;
    sc_out<bool>     rst;
    sc_in<bool>      rst_in;

    SC_CTOR(tb_driver)
    {
        SC_CTHREAD(source, clk.pos());
        reset_signal_is(rst_in, 0);

        SC_CTHREAD(sink, clk.pos());
        reset_signal_is(rst_in, 0);

        rst_in(rst);
    }

    void source();
    void sink();
};

#endif
