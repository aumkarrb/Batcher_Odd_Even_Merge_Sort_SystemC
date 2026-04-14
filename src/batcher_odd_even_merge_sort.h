/******************************************************************************
 * 
 *
 * Batcher Odd-Even Merge Sort  -  8-input static sorting network
 * 
 *
 * ---------------------------------------------------------------------
 *  NETWORK TOPOLOGY  -  19 comparators across 6 pipeline stages
 * ---------------------------------------------------------------------
 *
 *  Stage 1 [Sort adj. pairs]  :  (0,1)  (2,3)  (4,5)  (6,7)   [4]
 *  Stage 2 [Sort stride-2]    :  (0,2)  (1,3)  (4,6)  (5,7)   [4]
 *  Stage 3 [Finish half-sorts]:  (1,2)  (5,6)                  [2]
 *     ^ After Stage 3: s[0..3] sorted  AND  s[4..7] sorted independently
 *  Stage 4 [Cross-half 1st]   :  (0,4)  (1,5)  (2,6)  (3,7)  [4]
 *  Stage 5 [Sub-merge inner]  :  (2,4)  (3,5)                  [2]
 *  Stage 6 [Final interleave] :  (1,2)  (3,4)  (5,6)           [3]
 *     ^ After Stage 6: s[0..7] fully sorted (ascending)
 *
 *  Total: 4 + 4 + 2 + 4 + 2 + 3 = 19 comparators
 *
 * ---------------------------------------------------------------------
 *  PIPELINE TIMING
 * ---------------------------------------------------------------------
 *  Cycle  0     : IDLE - awaiting in_valid
 *  Cycle  1     : Input capture
 *  Cycles 2-7   : Stages 1-6 (one per clock cycle)
 *  Cycle  8     : Outputs driven, out_valid asserted (1-cycle pulse)
 *  Total latency: 8 clock cycles from in_valid to out_valid
 *
 ******************************************************************************/

#ifndef ODD_EVEN_MERGE_SORT_H
#define ODD_EVEN_MERGE_SORT_H

#ifndef SC_DISABLE_API_VERSION_CHECK
#define SC_DISABLE_API_VERSION_CHECK
#endif

#include <systemc.h>

typedef sc_int<16> data_t;

// ============================================================================
//  DUT Module  :  odd_even_merge_sort
// ============================================================================
SC_MODULE(odd_even_merge_sort) {

    // -------------------------------------------------------------------------
    //  Port list
    // -------------------------------------------------------------------------
    sc_in<bool>    clk;        // System clock  -  rising-edge triggered
    sc_in<bool>    rst;        // Synchronous reset, active-HIGH

    sc_in<data_t>  din[8];     // Unsorted input values
    sc_in<bool>    in_valid;   // Upstream handshake: pulse HIGH for one cycle
                               //   to present a valid input vector

    sc_out<data_t> dout[8];    // Sorted output values (ascending)
    sc_out<bool>   out_valid;  // Downstream handshake: pulses HIGH for one
                               //   cycle when sorted output is valid

    // -------------------------------------------------------------------------
    //  Constructor / process registration
    // -------------------------------------------------------------------------
    SC_CTOR(odd_even_merge_sort) {
        SC_THREAD(sort_process);
        sensitive << clk.pos();
        dont_initialize();     // suppress spurious call before first clock edge
    }

    // -------------------------------------------------------------------------
    //  Private helpers & processes  (defined in odd_even_merge_sort.cpp)
    // -------------------------------------------------------------------------
private:
    inline void compare_and_swap(data_t &a, data_t &b);
    void sort_process();
};

#endif 
