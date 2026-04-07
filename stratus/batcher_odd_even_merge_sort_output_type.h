#ifndef BATCHER_ODD_EVEN_MERGE_SORT_OUTPUT_TYPE_H
#define BATCHER_ODD_EVEN_MERGE_SORT_OUTPUT_TYPE_H

#include <systemc.h>
#include <cynw_p2p.h>

// Must match DUT: 8 sorted outputs, sc_int<16> each
struct batcher_odd_even_merge_sort_OUTPUT_DT
{
    sc_int<16> data[8];

    inline bool operator==(const batcher_odd_even_merge_sort_OUTPUT_DT& rhs) const {
        for (int i = 0; i < 8; i++)
            if (data[i] != rhs.data[i]) return false;
        return true;
    }

    inline batcher_odd_even_merge_sort_OUTPUT_DT&
    operator=(const batcher_odd_even_merge_sort_OUTPUT_DT& rhs) {
        for (int i = 0; i < 8; i++)
            data[i] = rhs.data[i];
        return *this;
    }

    inline friend void sc_trace(sc_trace_file* tf,
                                const batcher_odd_even_merge_sort_OUTPUT_DT& v,
                                const std::string& name) {
        for (int i = 0; i < 8; i++)
            sc_trace(tf, v.data[i], name + ".data[" + std::to_string(i) + "]");
    }

    inline friend std::ostream& operator<<(std::ostream& os,
                                           const batcher_odd_even_merge_sort_OUTPUT_DT& v) {
        os << "{";
        for (int i = 0; i < 8; i++) {
            os << v.data[i].to_int();
            if (i < 7) os << ",";
        }
        return os << "}";
    }
};

#endif
