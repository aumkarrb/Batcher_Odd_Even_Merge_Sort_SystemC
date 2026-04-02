/******************************************************************************
 * tb_main.cpp
 *
 * Batcher Odd-Even Merge Sort  -  Testbench implementation + sc_main
 *
 * Build (example, GCC + SystemC):
 *   g++ -std=c++14 -DSC_DISABLE_API_VERSION_CHECK \
 *       -I$SYSTEMC_HOME/include \
 *       -L$SYSTEMC_HOME/lib-linux64 -lsystemc \
 *       odd_even_merge_sort.cpp tb_main.cpp -o sim
 *
 * Optional waveform dump:
 *   Compile with -DDUMP_VCD to emit odd_even_sort_wave.vcd
 ******************************************************************************/

#include "tb_driver.h"

// ============================================================================
//  tb_driver::stimulus
//  Applies synchronous reset, then submits NUM_TESTS input vectors
//  sequentially.  Waits 12 cycles between each vector (8-cycle latency + 4
//  guard cycles) before submitting the next one.
// ============================================================================
void tb_driver::stimulus() {
    using namespace std;

    // ---- Assert reset -------------------------------------------------------
    rst.write(true);
    in_valid.write(false);
    for (int i = 0; i < 8; i++)
        din[i].write(data_t(0));

    for (int c = 0; c < 4; c++) wait();   // hold reset for 4 cycles
    rst.write(false);

    wait(); wait();                        // two idle cycles before first test

    // ---- Test vectors -------------------------------------------------------
    // sc_int<16> range: -32768 to 32767
    struct TC { const char *label; sc_int<16> v[8]; };

    const TC tests[] = {
        { "Typical mixed positive/negative",
          { 34, -7, 99, 0, -128, 55, 21, 8 } },

        { "Already sorted (best case)",
          { -100, -5, 1, 3, 12, 47, 88, 200 } },

        { "Reverse sorted (worst case)",
          { 200, 88, 47, 12, 3, 1, -5, -100 } },

        { "All identical values",
          { 42, 42, 42, 42, 42, 42, 42, 42 } },

        { "Near-extreme sc_int<16> values",
          { 32767, -32768, 0, -1, 1, -32767, 32766, -2 } },

        { "Many duplicates",
          { 5, 3, 5, 1, 3, 7, 1, 7 } },

        { "Alternating high/low",
          { 400, -400, 300, -300, 200, -200, 100, -100 } },

        { "Single unique minimum element",
          { 9, 8, 7, 6, 5, 4, 3, -999 } },
    };

    const int NUM_TESTS = (int)(sizeof(tests) / sizeof(tests[0]));

    cout << "  Submitting " << NUM_TESTS << " test vectors...\n\n";

    for (int t = 0; t < NUM_TESTS; t++) {

        // Cache inputs so monitor() can build the reference vector
        for (int i = 0; i < 8; i++)
            test_inputs[t][i] = tests[t].v[i];

        cout << "  +- Test " << setw(2) << (t + 1)
             << ": " << tests[t].label << "\n";
        cout << "  |  Input : [ ";
        for (int i = 0; i < 8; i++) {
            cout << setw(7) << tests[t].v[i].to_int();
            if (i < 7) cout << ", ";
        }
        cout << " ]\n";

        current_test = t;

        // Present the input vector for exactly one cycle
        for (int i = 0; i < 8; i++)
            din[i].write(tests[t].v[i]);
        in_valid.write(true);
        wait();
        in_valid.write(false);

        // Wait for pipeline to complete + guard cycles
        for (int c = 0; c < 12; c++) wait();
    }

    // Allow monitor() to catch the last result
    for (int c = 0; c < 6; c++) wait();

    // ---- Final summary ------------------------------------------------------
    cout << "\n  ======================================================\n";
    cout << "  SIMULATION COMPLETE\n";
    cout << "  Tests run   : " << tests_run    << "\n";
    cout << "  Tests passed: " << tests_passed << "\n";
    cout << "  Tests failed: " << (tests_run - tests_passed) << "\n";
    cout << "  ======================================================\n\n";

    sc_stop();
}

// ============================================================================
//  tb_driver::monitor
//  Watches out_valid; on each rising-edge assertion it captures dout[],
//  sorts a copy of the corresponding input vector with std::sort, and
//  compares element-by-element.  Also verifies monotonicity of the output.
// ============================================================================
void tb_driver::monitor() {
    using namespace std;

    int result_idx = 0;   // tracks which test result we are checking

    while (true) {
        wait();
        if (!out_valid.read()) continue;

        tests_run++;

        // Capture DUT output
        sc_int<16> got[8];
        for (int i = 0; i < 8; i++)
            got[i] = dout[i].read();

        // Build reference via std::sort
        int ref[8];
        for (int i = 0; i < 8; i++)
            ref[i] = test_inputs[result_idx][i].to_int();
        std::sort(ref, ref + 8);

        // Check correctness (value match) and monotonicity (safety net)
        bool pass = true;
        for (int i = 0; i < 8; i++)
            if (got[i].to_int() != ref[i]) { pass = false; break; }
        for (int i = 0; i < 7 && pass; i++)
            if (got[i] > got[i + 1]) { pass = false; break; }

        if (pass) tests_passed++;

        cout << "  |  Output: [ ";
        for (int i = 0; i < 8; i++) {
            cout << setw(7) << got[i].to_int();
            if (i < 7) cout << ", ";
        }
        cout << " ]\n";
        cout << "  +- Result: " << (pass ? "PASS [OK]" : "FAIL [!!]") << "\n\n";

        result_idx++;
    }
}

// ============================================================================
//  sc_main
//  Instantiates the clock, signals, DUT, and testbench; wires everything
//  together; optionally enables VCD waveform dumping; then starts simulation.
// ============================================================================
int sc_main(int argc, char *argv[]) {
    using namespace std;

    cout << "\n";
    cout << "  ======================================================\n";
    cout << "   Batcher Odd-Even Merge Sort -- 8-input sorting network\n";
    cout << "   SystemC HLS-Compatible Implementation\n";
    cout << "   Data type : sc_int<16>   |  Clock period : 10 ns\n";
    cout << "   Pipeline latency  : 8 clock cycles\n";
    cout << "   Comparator stages : 6    |  Total comparators : 19\n";
    cout << "  ======================================================\n\n";

    // ---- Clock --------------------------------------------------------------
    sc_clock clk("clk", 10, SC_NS, 0.5, 0, SC_NS, false);

    // ---- Signals ------------------------------------------------------------
    sc_signal<bool>   sig_rst       ("sig_rst");
    sc_signal<bool>   sig_in_valid  ("sig_in_valid");
    sc_signal<bool>   sig_out_valid ("sig_out_valid");
    sc_signal<data_t> sig_din[8];
    sc_signal<data_t> sig_dout[8];

    // Assign distinct names for waveform readability
    for (int i = 0; i < 8; i++) {
        char nm_in[16], nm_out[16];
        snprintf(nm_in,  sizeof(nm_in),  "sig_din_%d",  i);
        snprintf(nm_out, sizeof(nm_out), "sig_dout_%d", i);
        sig_din [i].name();
        sig_dout[i].name();
    }

    // ---- DUT instantiation & port binding -----------------------------------
    odd_even_merge_sort dut("dut");
    dut.clk      (clk);
    dut.rst      (sig_rst);
    dut.in_valid (sig_in_valid);
    dut.out_valid(sig_out_valid);
    for (int i = 0; i < 8; i++) {
        dut.din [i](sig_din [i]);
        dut.dout[i](sig_dout[i]);
    }

    // ---- Testbench instantiation & port binding -----------------------------
    tb_driver tb("tb");
    tb.clk      (clk);
    tb.rst      (sig_rst);
    tb.in_valid (sig_in_valid);
    tb.out_valid(sig_out_valid);
    for (int i = 0; i < 8; i++) {
        tb.din [i](sig_din [i]);
        tb.dout[i](sig_dout[i]);
    }

    // ---- Optional VCD waveform dump -----------------------------------------
#ifdef DUMP_VCD
    sc_trace_file *tf = sc_create_vcd_trace_file("odd_even_sort_wave");
    tf->set_time_unit(1, SC_NS);

    sc_trace(tf, clk,            "clk");
    sc_trace(tf, sig_rst,        "rst");
    sc_trace(tf, sig_in_valid,   "in_valid");
    sc_trace(tf, sig_out_valid,  "out_valid");
    for (int i = 0; i < 8; i++) {
        std::string dn = std::string("din[")  + char('0' + i) + "]";
        std::string qn = std::string("dout[") + char('0' + i) + "]";
        sc_trace(tf, sig_din [i], dn);
        sc_trace(tf, sig_dout[i], qn);
    }
#endif

    // ---- Run simulation -----------------------------------------------------
    sc_start();

#ifdef DUMP_VCD
    sc_close_vcd_trace_file(tf);
#endif

    return 0;
}
