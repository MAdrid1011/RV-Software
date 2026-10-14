/* Reuse upstream console, FP bit conversion and environment functions. The
 * bare-metal port supplies cycle timing and has no host file redirection.
 */
#undef FAKE_FILEIO
#define FAKE_FILEIO 0
#define al_signal_start host_signal_start
#define al_signal_finished host_signal_finished
#define al_signal_now host_signal_now
#define al_ticks_per_sec host_ticks_per_sec
#define al_tick_granularity host_tick_granularity
#define al_main host_main
#include "upstream/mith/al/src/th_al.c"
#undef al_signal_start
#undef al_signal_finished
#undef al_signal_now
#undef al_ticks_per_sec
#undef al_tick_granularity
#undef al_main
#include "../simd/runtime/runtime.h"

static uint64_t started;
void al_signal_start(void) { started = simd_cycle64(); }
size_t al_signal_finished(void) { return (size_t)(simd_cycle64() - started); }
size_t al_signal_now(void) { return (size_t)(simd_cycle64() - started); }
size_t al_ticks_per_sec(void) { return 1000000; }
size_t al_tick_granularity(void) { return 1; }
void al_main(int argc, char **argv) { (void)argc; (void)argv; }
