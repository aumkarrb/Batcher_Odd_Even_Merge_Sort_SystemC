#include "tb_driver.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

void tb_driver::source()
{
    outputs.reset();
    rst = 0;
    wait(4);
    rst = 1;
    wait(2);

    sc_int<16> test_vectors[8][8] = {
        {  34,  -7,  99,   0, -128,  55,  21,   8 },
        {-100,  -5,   1,   3,   12,  47,  88, 200 },
        { 200,  88,  47,  12,    3,   1,  -5,-100 },
        {  42,  42,  42,  42,   42,  42,  42,  42 },
        {32767,-32768, 0, -1,    1,-32767,32766, -2},
        {   5,   3,   5,   1,    3,   7,   1,   7 },
        { 400,-400, 300,-300,  200,-200, 100,-100 },
        {   9,   8,   7,   6,    5,   4,   3,-999 }
    };

    for (int t = 0; t < 8; t++)
    {
        batcher_odd_even_merge_sort_INPUT_DT pkt;
        for (int i = 0; i < 8; i++)
            pkt.data[i] = test_vectors[t][i];

        std::cout << "  [SRC] Sending test " << (t+1) << ": [ ";
        for (int i = 0; i < 8; i++) {
            std::cout << std::setw(7) << test_vectors[t][i].to_int();
            if (i < 7) std::cout << ",";
        }
        std::cout << " ]\n";

        outputs.put(pkt);
    }
}

void tb_driver::sink()
{
    inputs.reset();
    wait();

    sc_int<16> test_vectors[8][8] = {
        {  34,  -7,  99,   0, -128,  55,  21,   8 },
        {-100,  -5,   1,   3,   12,  47,  88, 200 },
        { 200,  88,  47,  12,    3,   1,  -5,-100 },
        {  42,  42,  42,  42,   42,  42,  42,  42 },
        {32767,-32768, 0, -1,    1,-32767,32766, -2},
        {   5,   3,   5,   1,    3,   7,   1,   7 },
        { 400,-400, 300,-300,  200,-200, 100,-100 },
        {   9,   8,   7,   6,    5,   4,   3,-999 }
    };

    int passed = 0;

    for (int t = 0; t < 8; t++)
    {
        batcher_odd_even_merge_sort_OUTPUT_DT pkt = inputs.get();

        // Build reference
        int ref[8];
        for (int i = 0; i < 8; i++) ref[i] = test_vectors[t][i].to_int();
        std::sort(ref, ref + 8);

        bool ok = true;
        for (int i = 0; i < 8; i++)
            if (pkt.data[i].to_int() != ref[i]) { ok = false; break; }

        std::cout << "  [SNK] Result  test " << (t+1) << ": [ ";
        for (int i = 0; i < 8; i++) {
            std::cout << std::setw(7) << pkt.data[i].to_int();
            if (i < 7) std::cout << ",";
        }
        std::cout << " ]  " << (ok ? "PASS" : "FAIL") << "\n";

        if (ok) passed++;
    }

    std::cout << "\n  ================================\n";
    std::cout << "  Results: " << passed << "/8 passed\n";
    std::cout << "  ================================\n\n";

    esc_stop();
}