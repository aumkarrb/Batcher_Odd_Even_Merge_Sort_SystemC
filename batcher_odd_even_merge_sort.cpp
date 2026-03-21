/******************************************************************************
 * odd_even_merge_sort.cpp
 *
 * Batcher Odd-Even Merge Sort - 8-input static sorting network
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

#ifndef SC_DISABLE_API_VERSION_CHECK
#define SC_DISABLE_API_VERSION_CHECK
#endif

#include <systemc.h>
#include <iostream>
#include <iomanip>
#include <algorithm>   
typedef sc_int<16> data_t;


SC_MODULE(odd_even_merge_sort) {

    sc_in<bool>    clk;        // System clock - rising-edge triggered
    sc_in<bool>    rst;        // Synchronous reset, active-HIGH

    // --- Input data bus (8 independent 16-bit values) ---
    sc_in<data_t>  din[8];     // Unsorted input values
    sc_in<bool>    in_valid;   // Upstream handshake: pulse HIGH for exactly one
                               //   clock cycle to present a valid input vector

    // --- Output data bus (8 independent 16-bit values) ---
    sc_out<data_t> dout[8];    // Sorted output values
    sc_out<bool>   out_valid;  // Downstream handshake: pulses HIGH for exactly
                               //   one clock cycle when sorted output is valid

        SC_CTOR(odd_even_merge_sort) {
        SC_THREAD(sort_process);
        sensitive << clk.pos();
        dont_initialize();     // suppress spurious call before first clock edge
    }


    inline void compare_and_swap(data_t &a, data_t &b) {
        if (a > b) {
            data_t tmp = a;
            a = b;
            b = tmp;
        }
    }


    void sort_process() {


        out_valid.write(false);
        for (int i = 0; i < 8; i++) {
            dout[i].write(data_t(0));
        }


        do {
            wait();
        } while (rst.read() == true);


        while (true) {


            out_valid.write(false);

            while (in_valid.read() == false) {
                wait();
            }


            data_t s[8];
            s[0] = din[0].read();
            s[1] = din[1].read();
            s[2] = din[2].read();
            s[3] = din[3].read();
            s[4] = din[4].read();
            s[5] = din[5].read();
            s[6] = din[6].read();
            s[7] = din[7].read();

            wait();  
            compare_and_swap(s[0], s[1]);    // Stage 1 . comparator (0,1)
            compare_and_swap(s[2], s[3]);    // Stage 1 . comparator (2,3)
            compare_and_swap(s[4], s[5]);    // Stage 1 . comparator (4,5)
            compare_and_swap(s[6], s[7]);    // Stage 1 . comparator (6,7)

            wait();  
            compare_and_swap(s[0], s[2]);    // Stage 2 . comparator (0,2)
            compare_and_swap(s[1], s[3]);    // Stage 2 . comparator (1,3)
            compare_and_swap(s[4], s[6]);    // Stage 2 . comparator (4,6)
            compare_and_swap(s[5], s[7]);    // Stage 2 . comparator (5,7)

            wait(); 
            compare_and_swap(s[1], s[2]);    // Stage 3 . comparator (1,2)
            compare_and_swap(s[5], s[6]);    // Stage 3 . comparator (5,6)

            wait(); 
            compare_and_swap(s[0], s[4]);    // Stage 4 . comparator (0,4)
            compare_and_swap(s[1], s[5]);    // Stage 4 . comparator (1,5)
            compare_and_swap(s[2], s[6]);    // Stage 4 . comparator (2,6)
            compare_and_swap(s[3], s[7]);    // Stage 4 . comparator (3,7)

            wait();  
            compare_and_swap(s[2], s[4]);    // Stage 5 . comparator (2,4)
            compare_and_swap(s[3], s[5]);    // Stage 5 . comparator (3,5)

            wait();  
            compare_and_swap(s[1], s[2]);    // Stage 6 . comparator (1,2)
            compare_and_swap(s[3], s[4]);    // Stage 6 . comparator (3,4)
            compare_and_swap(s[5], s[6]);    // Stage 6 . comparator (5,6)

            wait();            
            dout[0].write(s[0]);
            dout[1].write(s[1]);
            dout[2].write(s[2]);
            dout[3].write(s[3]);
            dout[4].write(s[4]);
            dout[5].write(s[5]);
            dout[6].write(s[6]);
            dout[7].write(s[7]);
            out_valid.write(true);

            wait();  

            
        } 
    } 

}; 


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

                // [sc_int<16> range: -32768 to 32767]
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
           for (int i = 0; i < 8; i++) {
                test_inputs[t][i] = tests[t].v[i];
            }

            
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
            for (int i = 0; i < 8; i++) {
                ref[i] = test_inputs[result_idx][i].to_int();
            }
            std::sort(ref, ref + 8);

            
            bool pass = true;
            for (int i = 0; i < 8; i++) {
                if (got[i].to_int() != ref[i]) { pass = false; break; }
            }
            for (int i = 0; i < 7 && pass; i++) {
                if (got[i] > got[i+1]) { pass = false; break; }
            }

            if (pass) tests_passed++;

 
            cout << "  |  Output: [ ";
            for (int i = 0; i < 8; i++) {
                cout << setw(7) << got[i].to_int();
                if (i < 7) cout << ", ";
            }
            cout << " ]\n";
            cout << "  +- Result: " << (pass ? "PASS [OK]" : "FAIL [FAIL]") << "\n\n";

            result_idx++;
        }
    }

}; 



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

   
    sc_clock clk("clk", 10, SC_NS, 0.5, 0, SC_NS, false);


    sc_signal<bool>    sig_rst       ("sig_rst");
    sc_signal<bool>    sig_in_valid  ("sig_in_valid");
    sc_signal<bool>    sig_out_valid ("sig_out_valid");
    sc_signal<data_t>  sig_din[8];
    sc_signal<data_t>  sig_dout[8];

    // Assign distinct names for waveform dumps
    for (int i = 0; i < 8; i++) {
        char nm[16];
    }


    odd_even_merge_sort dut("dut");
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
    sc_trace_file *tf = sc_create_vcd_trace_file("odd_even_sort_wave");
    tf->set_time_unit(1, SC_NS);
    sc_trace(tf, clk,            "clk");
    sc_trace(tf, sig_rst,        "rst");
    sc_trace(tf, sig_in_valid,   "in_valid");
    sc_trace(tf, sig_out_valid,  "out_valid");
    for (int i = 0; i < 8; i++) {
        std::string dn = std::string("din[")  + char('0'+i) + "]";
        std::string qn = std::string("dout[") + char('0'+i) + "]";
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


