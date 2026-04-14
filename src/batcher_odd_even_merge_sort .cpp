/******************************************************************************
 * Batcher Odd-Even Merge Sort - 8-input static sorting network
 *
 * ---------------------------------------------------------------------
 *  NETWORK TOPOLOGY  -  19 comparators across 6 pipeline stages
 * ---------------------------------------------------------------------
 *  Stage 1 [Sort adj. pairs]  :  (0,1)  (2,3)  (4,5)  (6,7)   [4]
 *  Stage 2 [Sort stride-2]    :  (0,2)  (1,3)  (4,6)  (5,7)   [4]
 *  Stage 3 [Finish half-sorts]:  (1,2)  (5,6)                  [2]
 *  Stage 4 [Cross-half 1st]   :  (0,4)  (1,5)  (2,6)  (3,7)   [4]
 *  Stage 5 [Sub-merge inner]  :  (2,4)  (3,5)                  [2]
 *  Stage 6 [Final interleave] :  (1,2)  (3,4)  (5,6)           [3]
 *  Total: 4+4+2+4+2+3 = 19 comparators
 *
 * ---------------------------------------------------------------------
 *  PIPELINE TIMING
 * ---------------------------------------------------------------------
 *  Cycle 0     : Reset / IDLE
 *  Cycle 1     : Input capture (in_valid sampled)
 *  Cycles 2-7  : Stages 1-6 (one stage per clock)
 *  Cycle 8     : Outputs written, out_valid pulsed HIGH
 *  Total latency: 8 clock cycles
 ******************************************************************************/

#ifndef SC_DISABLE_API_VERSION_CHECK
#define SC_DISABLE_API_VERSION_CHECK
#endif

#include <systemc.h>

typedef sc_int<16> data_t;

// =============================================================================
//  DUT: batcher_odd_even_merge_sort
// =============================================================================

SC_MODULE(batcher_odd_even_merge_sort) {

    // ── Ports ─────────────────────────────────────────────────────────────────
    sc_in<bool>    clk;
    sc_in<bool>    rst;

    sc_in<data_t>  din[8];
    sc_in<bool>    in_valid;

    sc_out<data_t> dout[8];
    sc_out<bool>   out_valid;

    // ── Constructor ───────────────────────────────────────────────────────────
    SC_CTOR(batcher_odd_even_merge_sort) {
        SC_CTHREAD(sort_process, clk.pos());   // clocked thread: required by Stratus
        reset_signal_is(rst, true);            // active-HIGH synchronous reset
    }

    // ── Process ───────────────────────────────────────────────────────────────
    void sort_process() {

        // ── Reset state ───────────────────────────────────────────────────────
        // Everything assigned here is the reset value of each register.

        out_valid.write(false);
        for (int i = 0; i < 8; i++) {
            dout[i].write(data_t(0));
        }
        wait();   // single wait() ends the reset state for SC_CTHREAD

        // ── Normal operation (runs every clock after reset deasserts) ─────────
        while (true) {

            out_valid.write(false);

            // Wait until upstream presents a valid input vector
            while (in_valid.read() == false) {
                wait();
            }

            // Capture all 8 inputs in the same cycle in_valid is seen
            data_t s[8];
            for (int i = 0; i < 8; i++) {
                s[i] = din[i].read();
            }

            // ── Stage 1: sort adjacent pairs ──────────────────────────────────
            wait();
            { data_t a=s[0],b=s[1]; if(a>b){s[0]=b;s[1]=a;} }
            { data_t a=s[2],b=s[3]; if(a>b){s[2]=b;s[3]=a;} }
            { data_t a=s[4],b=s[5]; if(a>b){s[4]=b;s[5]=a;} }
            { data_t a=s[6],b=s[7]; if(a>b){s[6]=b;s[7]=a;} }

            // ── Stage 2: sort stride-2 pairs ──────────────────────────────────
            wait();
            { data_t a=s[0],b=s[2]; if(a>b){s[0]=b;s[2]=a;} }
            { data_t a=s[1],b=s[3]; if(a>b){s[1]=b;s[3]=a;} }
            { data_t a=s[4],b=s[6]; if(a>b){s[4]=b;s[6]=a;} }
            { data_t a=s[5],b=s[7]; if(a>b){s[5]=b;s[7]=a;} }

            // ── Stage 3: finish independent half-sorts ────────────────────────
            wait();
            { data_t a=s[1],b=s[2]; if(a>b){s[1]=b;s[2]=a;} }
            { data_t a=s[5],b=s[6]; if(a>b){s[5]=b;s[6]=a;} }

            // ── Stage 4: cross-half merge (first pass) ────────────────────────
            wait();
            { data_t a=s[0],b=s[4]; if(a>b){s[0]=b;s[4]=a;} }
            { data_t a=s[1],b=s[5]; if(a>b){s[1]=b;s[5]=a;} }
            { data_t a=s[2],b=s[6]; if(a>b){s[2]=b;s[6]=a;} }
            { data_t a=s[3],b=s[7]; if(a>b){s[3]=b;s[7]=a;} }

            // ── Stage 5: sub-merge inner pairs ────────────────────────────────
            wait();
            { data_t a=s[2],b=s[4]; if(a>b){s[2]=b;s[4]=a;} }
            { data_t a=s[3],b=s[5]; if(a>b){s[3]=b;s[5]=a;} }

            // ── Stage 6: final interleave ─────────────────────────────────────
            wait();
            { data_t a=s[1],b=s[2]; if(a>b){s[1]=b;s[2]=a;} }
            { data_t a=s[3],b=s[4]; if(a>b){s[3]=b;s[4]=a;} }
            { data_t a=s[5],b=s[6]; if(a>b){s[5]=b;s[6]=a;} }

            // ── Write sorted outputs ──────────────────────────────────────────
            wait();
            for (int i = 0; i < 8; i++) {
                dout[i].write(s[i]);
            }
            out_valid.write(true);

            wait();   // hold out_valid for exactly one cycle then loop back

        } // while(true)
    } // sort_process

}; // SC_MODULE


// =============================================================================
//  TESTBENCH 
// =============================================================================

#ifndef STRATUS_HLS

#include <iostream>
#include <iomanip>
#include <algorithm>

SC_MODULE(tb_driver) {
    sc_in<bool>    clk;

    sc_out<bool>   rst;
    sc_out<data_t> din[8];
    sc_out<bool>   in_valid;

    sc_in<data_t>  dout[8];
    sc_in<bool>    out_valid;

    int tests_run;
    int tests_passed;

    sc_int<16> test_inputs[8][8];
    int        current_test;

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

    void stimulus() {
        using namespace std;

        rst.write(true);
        in_valid.write(false);
        for (int i = 0; i < 8; i++) din[i].write(data_t(0));

        for (int c = 0; c < 4; c++) wait();
        rst.write(false);
        wait(); wait();

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
            for (int i = 0; i < 8; i++)
                test_inputs[t][i] = tests[t].v[i];

            cout << "  +- Test " << setw(2) << (t+1)
                 << ": " << tests[t].label << "\n";
            cout << "  |  Input : [ ";
            for (int i = 0; i < 8; i++) {
                cout << setw(7) << tests[t].v[i].to_int();
                if (i < 7) cout << ", ";
            }
            cout << " ]\n";

            current_test = t;

            for (int i = 0; i < 8; i++) din[i].write(tests[t].v[i]);
            in_valid.write(true);
            wait();
            in_valid.write(false);

            for (int c = 0; c < 12; c++) wait();
        }

        for (int c = 0; c < 6; c++) wait();

        cout << "\n  ======================================================\n";
        cout << "  SIMULATION COMPLETE\n";
        cout << "  Tests run   : " << tests_run    << "\n";
        cout << "  Tests passed: " << tests_passed << "\n";
        cout << "  Tests failed: " << (tests_run - tests_passed) << "\n";
        cout << "  ======================================================\n\n";

        sc_stop();
    }

    void monitor() {
        using namespace std;
        int result_idx = 0;

        while (true) {
            wait();
            if (!out_valid.read()) continue;

            tests_run++;

            sc_int<16> got[8];
            for (int i = 0; i < 8; i++) got[i] = dout[i].read();

            int ref[8];
            for (int i = 0; i < 8; i++)
                ref[i] = test_inputs[result_idx][i].to_int();
            std::sort(ref, ref + 8);

            bool pass = true;
            for (int i = 0; i < 8; i++)
                if (got[i].to_int() != ref[i]) { pass = false; break; }
            for (int i = 0; i < 7 && pass; i++)
                if (got[i] > got[i+1]) { pass = false; break; }

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
};

int sc_main(int argc, char *argv[]) {
    using namespace std;

    cout << "\n";
    cout << "  ======================================================\n";
    cout << "   Batcher Odd-Even Merge Sort -- 8-input sorting network\n";
    cout << "   Data type : sc_int<16>  |  Clock period : 20 ns\n";
    cout << "   Pipeline latency : 8 cycles  |  Comparators : 19\n";
    cout << "  ======================================================\n\n";

    sc_clock clk("clk", 10, SC_NS, 0.5, 0, SC_NS, false);

    sc_signal<bool>   sig_rst;
    sc_signal<bool>   sig_in_valid;
    sc_signal<bool>   sig_out_valid;
    sc_signal<data_t> sig_din[8];
    sc_signal<data_t> sig_dout[8];

    batcher_odd_even_merge_sort dut("dut");
    dut.clk      (clk);
    dut.rst      (sig_rst);
    dut.in_valid (sig_in_valid);
    dut.out_valid(sig_out_valid);
    for (int i = 0; i < 8; i++) {
        dut.din [i](sig_din [i]);
        dut.dout[i](sig_dout[i]);
    }

    tb_driver tb("tb");
    tb.clk      (clk);
    tb.rst      (sig_rst);
    tb.in_valid (sig_in_valid);
    tb.out_valid(sig_out_valid);
    for (int i = 0; i < 8; i++) {
        tb.din [i](sig_din [i]);
        tb.dout[i](sig_dout[i]);
    }

#ifdef DUMP_VCD
    sc_trace_file *tf = sc_create_vcd_trace_file("batcher_odd_even_sort_wave");
    tf->set_time_unit(1, SC_NS);
    sc_trace(tf, clk,           "clk");
    sc_trace(tf, sig_rst,       "rst");
    sc_trace(tf, sig_in_valid,  "in_valid");
    sc_trace(tf, sig_out_valid, "out_valid");
    for (int i = 0; i < 8; i++) {
        std::string dn = std::string("din_")  + char('0'+i);
        std::string qn = std::string("dout_") + char('0'+i);
        sc_trace(tf, sig_din [i], dn);
        sc_trace(tf, sig_dout[i], qn);
    }
#endif

    sc_start();

#ifdef DUMP_VCD
    sc_close_vcd_trace_file(tf);
#endif

    return 0;
}

#endif // STRATUS_HLS
