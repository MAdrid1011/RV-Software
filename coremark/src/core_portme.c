#include "coremark.h"

#if VALIDATION_RUN
	volatile ee_s32 seed1_volatile=0x3415;
	volatile ee_s32 seed2_volatile=0x3415;
	volatile ee_s32 seed3_volatile=0x66;
#endif
#if PERFORMANCE_RUN
	volatile ee_s32 seed1_volatile=0x0;
	volatile ee_s32 seed2_volatile=0x0;
	volatile ee_s32 seed3_volatile=0x66;
#endif
#if PROFILE_RUN
	volatile ee_s32 seed1_volatile=0x8;
	volatile ee_s32 seed2_volatile=0x8;
	volatile ee_s32 seed3_volatile=0x8;
#endif
	volatile ee_s32 seed4_volatile=ITERATIONS;
	volatile ee_s32 seed5_volatile=0;
/* Porting : Timing functions
	How to capture time and convert to seconds must be ported to whatever is supported by the platform.
	e.g. Read value from on board RTC, read value from cpu clock cycles performance counter etc.
	Sample implementation for standard time.h and windows.h definitions included.
*/
/* Define : TIMER_RES_DIVIDER
	Divider to trade off timer resolution and total time that can be measured.

	Use lower values to increase resolution, but make sure that overflow does not occur.
	If there are issues with the return value overflowing, increase this value.
	*/
static CORE_TICKS read_cycle(void) {
    CORE_TICKS cycles;
    __asm__ volatile("csrr %0, mcycle" : "=r"(cycles));
    return cycles;
}

/** Define Host specific (POSIX), or target specific global time variables. */
static CORE_TICKS start_time_val;
static CORE_TICKS stop_time_val;

/* Function : start_time
	This function will be called right before starting the timed portion of the benchmark.

	Implementation may be capturing a system timer (as implemented in the example code)
	or zeroing some system parameters - e.g. setting the cpu clocks cycles to 0.
*/
void start_time(void) {
    start_time_val = read_cycle();
}
/* Function : stop_time
	This function will be called right after ending the timed portion of the benchmark.

	Implementation may be capturing a system timer (as implemented in the example code)
	or other system parameters - e.g. reading the current value of cpu cycles counter.
*/
void stop_time(void) {
    stop_time_val = read_cycle();
}
/* Function : get_time
	Return an abstract "ticks" number that signifies time on the system.

	Actual value returned may be cpu cycles, milliseconds or any other value,
	as long as it can be converted to seconds by <time_in_secs>.
	This methodology is taken to accomodate any hardware or simulated platform.
	The sample implementation returns millisecs by default,
	and the resolution is controlled by <TIMER_RES_DIVIDER>
*/
CORE_TICKS get_time(void) {
  return stop_time_val - start_time_val;
}

/* Function : time_in_secs
	Convert the value returned by get_time to seconds.

	The <secs_ret> type is used to accomodate systems with no support for floating point.
	Default implementation implemented by the EE_TICKS_PER_SEC macro above.
*/
secs_ret time_in_secs(CORE_TICKS ticks) {
    return ticks;
}

typedef struct {
    ee_u32 high;
    ee_u32 low;
} coremark_u64_parts;

static coremark_u64_parts multiply_u32(ee_u32 lhs, ee_u32 rhs) {
    const ee_u32 lhs_low = lhs & 0xffffU;
    const ee_u32 lhs_high = lhs >> 16;
    const ee_u32 rhs_low = rhs & 0xffffU;
    const ee_u32 rhs_high = rhs >> 16;
    const ee_u32 product_low = lhs_low * rhs_low;
    const ee_u32 product_middle =
        (product_low >> 16) + (lhs_low * rhs_high & 0xffffU) + (lhs_high * rhs_low & 0xffffU);
    coremark_u64_parts product;

    product.low = (product_low & 0xffffU) | (product_middle << 16);
    product.high = lhs_high * rhs_high + (lhs_low * rhs_high >> 16) + (lhs_high * rhs_low >> 16)
        + (product_middle >> 16);
    return product;
}

static int less_than_or_equal(coremark_u64_parts lhs, coremark_u64_parts rhs) {
    return lhs.high < rhs.high || (lhs.high == rhs.high && lhs.low <= rhs.low);
}

ee_u32 coremark_score_milli(CORE_TICKS cycles, ee_u32 iterations) {
    const coremark_u64_parts scaled_iterations = multiply_u32(iterations, 1000000000U);
    ee_u32 low = 0;
    ee_u32 high = 1000000000U;

    if (cycles == 0U) {
        return 0U;
    }
    while (low < high) {
        const ee_u32 midpoint = low + (high - low + 1U) / 2U;
        if (less_than_or_equal(multiply_u32(midpoint, cycles), scaled_iterations)) {
            low = midpoint;
        } else {
            high = midpoint - 1U;
        }
    }
    return low;
}

ee_u32 default_num_contexts=1;

/* Function : portable_init
	Target specific initialization code
	Test for some common mistakes.
*/
void portable_init(core_portable *p, int *argc, char *argv[])
{
	ioe_init();
	if (sizeof(ee_ptr_int) != sizeof(ee_u8 *)) {
		ee_printf("ERROR! Please define ee_ptr_int to a type that holds a pointer!\n");
	}
	if (sizeof(ee_u32) != 4) {
		ee_printf("ERROR! Please define ee_u32 to a 32b unsigned type!\n");
	}
	p->portable_id=1;
}
/* Function : portable_fini
	Target specific final code
*/
void portable_fini(core_portable *p)
{
	p->portable_id=0;
}
