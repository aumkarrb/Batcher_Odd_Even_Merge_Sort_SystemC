/******************************************************************************
 * tb_driver.h
 *
 * Batcher Odd-Even Merge Sort  -  Self-checking SystemC Testbench
 * Header / module declaration
 *
 * The testbench consists of two co-operating SC_THREADs:
 *
 *   stimulus()  -  Drives reset, then submits NUM_TESTS input vectors
 *                  one at a time, waiting 12 cycles between submissions
 *                  to accommodate the 8-cycle pipeline latency plus margin.
 *
 *   monitor()   -  Watches out_valid; on each assertion captures dout[],
 *                  compares against std::sort reference, and prints
 *                  PASS / FAIL with a final summary report.
 *
 * Tests exercised (8 vectors):
 *   1. Typical mixed positive / negative values
 *   2. Already sorted (best case)
 *   3. Reverse sorted (worst case)
 *   4. All identical values
 *   5. Near-extreme sc_int<16> values (+/-32767, +/-32768)
 *   6. Many duplicate values
 *   7. Alternating high / low
 *   8. Single unique minimum element
 ******************************************************************************/

#ifndef TB_DRIVER_H
#define TB_DRIVER_H

#ifndef SC_DISABLE_API_VERSION_CHECK
#define SC_DISABLE_API_VERSION_CHECK
#endif

#include <systemc.h>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include "batcher_odd_even_merge_sort.h"    // pulls in data_t typedef

// ============================================================================
//  Testbench module
// ============================================================================
SC_MODULE(tb_driver) {

    // -------------------------------------------------------------------------
    //  Port list  (mirrors DUT, with directions swapped)
    // -------------------------------------------------------------------------
    sc_in<bool>    clk;

    sc_out<bool>   rst;
    sc_out<data_t> din[8];
    sc_out<bool>   in_valid;

    sc_in<data_t>  dout[8];
    sc_in<bool>    out_valid;

    // -------------------------------------------------------------------------
    //  Internal accounting
    // -------------------------------------------------------------------------
    int tests_run;
    int tests_passed;

    // Shared storage: stimulus() fills rows, monitor() reads them for reference
    sc_int<16> test_inputs[8][8];
    int        current_test;        // index of the test currently being driven

    // -------------------------------------------------------------------------
    //  Constructor / process registration
    // -------------------------------------------------------------------------
    SC_CTOR(tb_driver)
        : tests_run(0), tests_passed(0), current_test(0)
    {
        SC_THREAD(stimulus);
        sensitive << clk.pos();
        dont_initialize();

        SC_THREAD(monitor);
        sensitive << clk.pos();
        dont_initialize();
    }

private:
    void stimulus();
    void monitor();
};

#endif // TB_DRIVER_H
