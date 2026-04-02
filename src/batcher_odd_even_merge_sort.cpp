/******************************************************************************
 * odd_even_merge_sort.cpp
 *
 * Batcher Odd-Even Merge Sort  -  8-input static sorting network
 * DUT implementation  -  Cadence Stratus HLS synthesis target
 *
 * Compile alongside:
 *   odd_even_merge_sort.h   (module declaration)
 *   tb_driver.h / tb_main.cpp  (simulation testbench only)
 ******************************************************************************/

#include "batcher_odd_even_merge_sort.h"

// ============================================================================
//  compare_and_swap
//  In-place swap of two data_t values so that a <= b after the call.
//  Implemented as a single conditional swap; synthesises to a MUX-based
//  comparator in hardware.
// ============================================================================
inline void odd_even_merge_sort::compare_and_swap(data_t &a, data_t &b) {
    if (a > b) {
        data_t tmp = a;
        a = b;
        b = tmp;
    }
}

// ============================================================================
//  sort_process
//  Main SC_THREAD.  Waits for in_valid, captures 8 inputs, pipelines them
//  through 6 comparator stages (one stage per clock cycle), then drives the
//  sorted outputs and asserts out_valid for exactly one cycle.
//
//  Cycle map (relative to the in_valid pulse):
//    +0  : in_valid sampled; inputs latched into s[]
//    +1  : Stage 1 comparators execute                 (wait #1)
//    +2  : Stage 2 comparators execute                 (wait #2)
//    +3  : Stage 3 comparators execute                 (wait #3)
//    +4  : Stage 4 comparators execute                 (wait #4)
//    +5  : Stage 5 comparators execute                 (wait #5)
//    +6  : Stage 6 comparators execute                 (wait #6)
//    +7  : dout[] driven, out_valid HIGH               (wait #7)
//    +8  : out_valid LOW; loop back to idle            (wait #8)
// ============================================================================
void odd_even_merge_sort::sort_process() {

    // ---- Reset / initialise outputs ----------------------------------------
    out_valid.write(false);
    for (int i = 0; i < 8; i++)
        dout[i].write(data_t(0));

    // ---- Wait for de-assertion of synchronous reset ------------------------
    do { wait(); } while (rst.read() == true);

    // ---- Main processing loop ----------------------------------------------
    while (true) {

        out_valid.write(false);

        // --- Idle: wait for upstream to assert in_valid ---------------------
        while (in_valid.read() == false)
            wait();

        // --- Capture inputs on the in_valid cycle ---------------------------
        data_t s[8];
        for (int i = 0; i < 8; i++)
            s[i] = din[i].read();

        // ===================================================================
        //  Pipeline stage 1  -  Sort adjacent pairs
        //  Comparators: (0,1)  (2,3)  (4,5)  (6,7)
        // ===================================================================
        wait();
        compare_and_swap(s[0], s[1]);
        compare_and_swap(s[2], s[3]);
        compare_and_swap(s[4], s[5]);
        compare_and_swap(s[6], s[7]);

        // ===================================================================
        //  Pipeline stage 2  -  Sort stride-2 pairs
        //  Comparators: (0,2)  (1,3)  (4,6)  (5,7)
        // ===================================================================
        wait();
        compare_and_swap(s[0], s[2]);
        compare_and_swap(s[1], s[3]);
        compare_and_swap(s[4], s[6]);
        compare_and_swap(s[5], s[7]);

        // ===================================================================
        //  Pipeline stage 3  -  Finish independent half-sorts
        //  Comparators: (1,2)  (5,6)
        //  Post-condition: s[0..3] sorted  AND  s[4..7] sorted
        // ===================================================================
        wait();
        compare_and_swap(s[1], s[2]);
        compare_and_swap(s[5], s[6]);

        // ===================================================================
        //  Pipeline stage 4  -  Cross-half merge (first pass)
        //  Comparators: (0,4)  (1,5)  (2,6)  (3,7)
        // ===================================================================
        wait();
        compare_and_swap(s[0], s[4]);
        compare_and_swap(s[1], s[5]);
        compare_and_swap(s[2], s[6]);
        compare_and_swap(s[3], s[7]);

        // ===================================================================
        //  Pipeline stage 5  -  Sub-merge inner elements
        //  Comparators: (2,4)  (3,5)
        // ===================================================================
        wait();
        compare_and_swap(s[2], s[4]);
        compare_and_swap(s[3], s[5]);

        // ===================================================================
        //  Pipeline stage 6  -  Final interleave
        //  Comparators: (1,2)  (3,4)  (5,6)
        //  Post-condition: s[0..7] fully sorted in ascending order
        // ===================================================================
        wait();
        compare_and_swap(s[1], s[2]);
        compare_and_swap(s[3], s[4]);
        compare_and_swap(s[5], s[6]);

        // ===================================================================
        //  Drive sorted outputs and assert out_valid for one cycle
        // ===================================================================
        wait();
        for (int i = 0; i < 8; i++)
            dout[i].write(s[i]);
        out_valid.write(true);

        // Pulse out_valid for exactly one clock cycle
        wait();
    }
}
