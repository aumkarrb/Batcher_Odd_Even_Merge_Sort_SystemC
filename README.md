# Batcher's Odd Even Merge Sort in SystemC
# Batcher's Odd-Even Merge Sort — SystemC / HLS

> **AHA Elective Mini-Project Assignment** · HLS-Synthesizable Batcher Odd-Even Merge Sort in SystemC, targeting Cadence Stratus High-Level Synthesis

![Language](https://img.shields.io/badge/Language-C%2B%2B%20%7C%20SystemC-blue)
![HLS Tool](https://img.shields.io/badge/HLS-Cadence%20Stratus%2020.2-orange)
![License](https://img.shields.io/badge/License-GPL--3.0-red)

---

## Overview

This project implements **Batcher's Odd-Even Merge Sort** as an HLS-synthesizable **SystemC** module targeting the **Cadence Stratus 20.2** High-Level Synthesis (HLS) tool. The design sorts 8 values of type `sc_int<16>` using a fixed 6-stage, 19-comparator sorting network with a fully pipelined, 8-cycle latency architecture — no runtime branches, no data-dependent control flow, entirely combinatorial compare-exchange stages.

The repository includes the synthesizable DUT, a standalone testbench for functional simulation, a Stratus TCL synthesis script, synthesis/timing/power/area reports, and waveform captures.

---

## Repository Structure

```
Batcher_Odd_Even_Merge_Sort_SystemC/
├── Reports/
│   ├── Area/
│   │   └── Area_Report.png                     # Post-synthesis area utilization report
│   ├── Power/
│   │   ├── Power_Report_1.png                  # Power analysis — full design breakdown
│   │   ├── Power_Report_2.png                  # Power analysis — leakage detail
│   │   ├── Power_Report_3.png                  # Power analysis — dynamic switching
│   │   ├── Power_Report_4.png                  # Power analysis — clock network
│   │   └── Power_Report_5.png                  # Power analysis — summary
│   ├── Report Summary/
│   │   ├── Basic_Simulation.png                # Functional simulation results screenshot
│   │   ├── Gating_Simulation.png               # Clock-gated simulation screenshot
│   │   ├── Report_Summary.png                  # Consolidated synthesis report overview
│   │   ├── Report_Summary_1.png                # Synthesis report detail — page 1
│   │   └── Report_Summary_2.png                # Synthesis report detail — page 2
│   └── Timing/
│       ├── P1.png                              # Timing path 1 (setup slack / path delay)
│       ├── P2.png                              # Timing path 2
│       ├── P3.png                              # Timing path 3
│       ├── P4.png                              # Timing path 4
│       ├── P5.png                              # Timing path 5
│       ├── P6.png                              # Timing path 6
│       ├── P7.png                              # Timing path 7
│       ├── P8.png                              # Timing path 8
│       ├── P9.png                              # Timing path 9
│       ├── P10.png                             # Timing path 10
│       ├── Critical_Path.png                   # Critical timing path through comparator chain
│       ├── Output.png                          # Simulation output terminal screenshot
│       ├── Output_2.png                        # Alternate simulation output view
│       ├── Synthesis.png                       # Stratus synthesis flow status screenshot
│       └── Waveform.jpeg                       # GTKWave waveform — sorted output signals
│
├── src/
│   ├── batcher_odd_even_merge_sort.cpp         # HLS-synthesizable DUT implementation
│   └── batcher_odd_even_merge_sort.h           # DUT module header (sc_module, ports, internals)
│
├── stratus/
│   ├── batcher_odd_even_merge_sort.cpp         # Stratus-compiled DUT copy (HLS project file)
│   ├── batcher_odd_even_merge_sort_i...        # Stratus intermediate / elaboration output
│   ├── batcher_odd_even_merge_sort_...         # Stratus intermediate / netlist output
│   ├── stratus_run.log                         # Full Stratus HLS run log (synthesis transcript)
│   ├── stratus_synth.tcl                       # Cadence Stratus synthesis TCL script
│   ├── tb_driver.cpp                           # Testbench driver (Stratus project copy)
│   └── tb_driver.h                             # Testbench driver header (Stratus project copy)
│
├── tb/
│   ├── tb_driver.h                             # Testbench stimulus driver header
│   └── tb_main.cpp                             # sc_main entry point; instantiates DUT + driver
│
├── LICENSE                                     # GNU General Public License v3.0
└── README.md                                   # This file
```

---

## Algorithm — Batcher's Odd-Even Merge Sort

Batcher's Odd-Even Merge Sort is a **sorting network** — a fixed, data-oblivious sequence of compare-exchange operations whose structure is determined entirely at design time, making it inherently parallel and ideal for hardware implementation.

For **8 inputs**, the network requires exactly **6 stages** and **19 comparators**, achieving O(log²n) depth:

```
Stage 1:  (0,1) (2,3) (4,5) (6,7)
Stage 2:  (0,2) (1,3) (4,6) (5,7)
Stage 3:  (1,2) (5,6)
Stage 4:  (0,4) (1,5) (2,6) (3,7)
Stage 5:  (2,4) (3,5)
Stage 6:  (1,2) (3,4) (5,6)
```

Each comparator implements: `if (a > b) swap(a, b)` — a single-cycle compare-exchange operation on `sc_int<16>` values.

| Property | Value |
|----------|-------|
| Input width | 8 elements |
| Data type | `sc_int<16>` |
| Comparators | 19 |
| Pipeline stages | 6 |
| Latency | 8 clock cycles (including I/O registration) |
| Throughput | 1 sort / clock (fully pipelined) |
| Sorting network depth | O(log²n) = 6 |
| Data dependency | None — fully data-oblivious |

---

## Design Architecture

```
tb_main.cpp (sc_main)
└── batcher_odd_even_merge_sort (SC_MODULE — DUT)
    ├── Port: clk (sc_in<bool>)
    ├── Port: rst (sc_in<bool>)
    ├── Ports: in[0..7] (sc_in<sc_int<16>>)
    ├── Ports: out[0..7] (sc_out<sc_int<16>>)
    └── Internal pipeline registers: stage_1..stage_6 signals

tb_driver (SC_MODULE — Stimulus)
├── Generates clock and reset
├── Applies randomized / directed test vectors to DUT inputs
└── Monitors and checks sorted outputs
```

The module is coded for HLS compatibility: sequential `SC_CTHREAD` / `SC_METHOD` with explicit clock and reset, `#pragma` hints for Stratus pipeline scheduling, and no dynamic memory allocation.

---

## HLS Synthesis — Cadence Stratus

The `stratus/stratus_synth.tcl` script drives the full HLS flow:

```tcl
# Key synthesis steps (representative)
set_project batcher_odd_even_merge_sort
set_top_design batcher_odd_even_merge_sort
set_clock clk -period 10        ;# 100 MHz target
set_library <CMOS_LIBRARY>
compile -rtl                    ;# Schedule and synthesize
report area
report timing
report power
```

---

## License

This project is licensed under the **GNU General Public License v3.0** — see [`LICENSE`](LICENSE) for details.

---
- Done as part of AHA Elective Mini-Project Assignment
