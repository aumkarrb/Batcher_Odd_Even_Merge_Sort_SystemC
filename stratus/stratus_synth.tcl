# =============================================================================
#  stratus_synth.tcl
#
#  Cadence Stratus HLS Synthesis Script
#  Design   : Batcher Odd-Even Merge Sort  (8-input, 16-bit, 6 pipeline stages)
#  DUT      : batcher_odd_even_merge_sort          (SC_MODULE name in the .cpp file)
#  Source   : batcher_odd_even_merge_sort.cpp  (single file: DUT + TB + sc_main)
#  Tool     : Cadence Stratus HLS 20.2  (/home/install/STRATUS202)
#  Machine  : pescad-01
#
#  Usage — batch  :  cd ~/PES1UG23EC066/stratus
#                    stratus -batch -file stratus_synth.tcl
#
#  Usage — interactive:
#                    cd ~/PES1UG23EC066/stratus
#                    stratus
#                    % source stratus_synth.tcl
#
#  ── CUSTOMISATION ────────────────────────────────────────────────────────────
#  Only edit SECTION 0 if anything changes (paths, clock period, etc.).
#  Never edit anything below SECTION 0.
# =============================================================================


# =============================================================================
#  SECTION 0 — USER CONFIGURATION  (only block you ever need to touch)
# =============================================================================

# ── Identity ──────────────────────────────────────────────────────────────────
#    TOP_MODULE must match the SC_MODULE(...) name exactly in the .cpp file
set TOP_MODULE    "batcher_odd_even_merge_sort"

#    Source .cpp filename (single combined file: DUT + TB + sc_main)
set SRC_FILE      "batcher_odd_even_merge_sort.cpp"

# ── Absolute project root ──────────────────────────────────────────────────────
set PROJECT_ROOT  "/home/cmos/PES1UG23EC066"

# ── Clock ─────────────────────────────────────────────────────────────────────
#    Must match the sc_in<bool> port name in your SC_MODULE exactly
set CLK_NAME      "clk"
set CLK_PERIOD    "10ns"

# ── Reset ─────────────────────────────────────────────────────────────────────
#    Must match the sc_in<bool> port name and polarity in your SC_MODULE
set RST_NAME      "rst"
#    active-HIGH synchronous reset (rst == true triggers reset in your code)

# ── HLS synthesis effort: low | medium | high ─────────────────────────────────
set EFFORT        "medium"

# ── HLS configuration name ────────────────────────────────────────────────────
#    Stratus creates <TOP_MODULE>_<CONFIG_NAME>/ for all output
set CONFIG_NAME   "BASIC"


# =============================================================================
#  SECTION 1 — DERIVED PATHS  (auto-computed, do not edit)
# =============================================================================

set SRC_DIR       "${PROJECT_ROOT}/src"
set STRATUS_DIR   "${PROJECT_ROOT}/stratus"
set REPORT_DIR    "${PROJECT_ROOT}/reports"
set CPP_FILE      "${SRC_DIR}/${SRC_FILE}"
set RTL_DIR       "${STRATUS_DIR}/${TOP_MODULE}_${CONFIG_NAME}/rtl"


# =============================================================================
#  SECTION 2 — PRE-FLIGHT CHECKS
# =============================================================================

puts ""
puts "============================================================"
puts "  Cadence Stratus HLS -- Pre-flight Check"
puts "============================================================"

if { ![file exists $CPP_FILE] } {
    puts ""
    puts "FATAL: Source file not found:"
    puts "       $CPP_FILE"
    puts ""
    puts "       Check PROJECT_ROOT and SRC_FILE in Section 0."
    puts ""
    exit 1
}
puts "OK  Source   : $CPP_FILE"

file mkdir $REPORT_DIR
puts "OK  Reports  : $REPORT_DIR"
puts "OK  Module   : $TOP_MODULE"
puts "OK  Clock    : ${CLK_NAME} @ ${CLK_PERIOD}"
puts "OK  Config   : ${CONFIG_NAME}  (effort: ${EFFORT})"
puts "============================================================"
puts ""


# =============================================================================
#  SECTION 3 — COMPILER FLAGS
#
#  -I              : tells Stratus where to find headers alongside the .cpp
#  -DSYNTHESIS     : CRITICAL for your file. Your .cpp contains both the DUT
#                    (odd_even_merge_sort) and the testbench (tb_driver) and
#                    sc_main in one file. The -DSYNTHESIS flag lets Stratus
#                    compile only the DUT by excluding TB-only code wrapped in:
#                        #ifndef SYNTHESIS
#                        ... tb_driver, sc_main ...
#                        #endif
#                    NOTE: If your source file does not have these guards yet,
#                    Stratus will still attempt synthesis of TOP_MODULE only,
#                    but adding the guards is strongly recommended (see below).
#
#  -DSC_DISABLE_API_VERSION_CHECK : suppresses SystemC version mismatch errors
# =============================================================================

set_attr sc_options \
    "-I${SRC_DIR} -DSYNTHESIS -DSC_DISABLE_API_VERSION_CHECK"


# =============================================================================
#  SECTION 4 — REGISTER THE HLS MODULE
#
#  Tells Stratus which SC_MODULE is the synthesis target.
#  The testbench class tb_driver and sc_main are ignored — Stratus only
#  elaborates and synthesises the module named in TOP_MODULE.
# =============================================================================

puts "Registering HLS module: $TOP_MODULE  <-  $CPP_FILE"
define_hls_module $TOP_MODULE $CPP_FILE


# =============================================================================
#  SECTION 5 — CLOCK AND RESET
# =============================================================================

puts "Defining clock  : ${CLK_NAME}  period = ${CLK_PERIOD}"
define_clock -name $CLK_NAME -period $CLK_PERIOD

puts "Defining reset  : ${RST_NAME}  active-HIGH"
define_reset $RST_NAME -high


# =============================================================================
#  SECTION 6 — HLS CONFIGURATION
# =============================================================================

puts "Defining config : $CONFIG_NAME"
define_hls_config $TOP_MODULE $CONFIG_NAME \
    -clock $CLK_NAME


# =============================================================================
#  SECTION 7 — SYNTHESIS
# =============================================================================

puts ""
puts "============================================================"
puts "  Starting HLS Synthesis"
puts "  Module  : $TOP_MODULE"
puts "  Config  : $CONFIG_NAME"
puts "  Effort  : $EFFORT"
puts "============================================================"
puts ""

synthesize \
    -d      $TOP_MODULE  \
    -config $CONFIG_NAME \
    -effort $EFFORT


# =============================================================================
#  SECTION 8 — POST-SYNTHESIS REPORTS
# =============================================================================

puts ""
puts "Writing reports to: $REPORT_DIR"

report timing  > ${REPORT_DIR}/timing.rpt
report area    > ${REPORT_DIR}/area.rpt
report latency > ${REPORT_DIR}/latency.rpt
report hier    > ${REPORT_DIR}/hierarchy.rpt


# =============================================================================
#  SECTION 9 — COMPLETION SUMMARY
# =============================================================================

puts ""
puts "============================================================"
puts "  Synthesis COMPLETE"
puts ""
puts "  RTL output :"
puts "    ${RTL_DIR}/"
puts ""
puts "  Reports :"
puts "    ${REPORT_DIR}/timing.rpt"
puts "    ${REPORT_DIR}/area.rpt"
puts "    ${REPORT_DIR}/latency.rpt"
puts "    ${REPORT_DIR}/hierarchy.rpt"
puts ""
puts "  Next -- RTL co-simulation:"
puts "    cd ${STRATUS_DIR}/${TOP_MODULE}_${CONFIG_NAME}"
puts "    make sim_rtl"
puts "============================================================"
puts ""
