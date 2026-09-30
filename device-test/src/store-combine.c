#include <devtest.h>

#define WC_BASE 0xa2000000u
#define ORDERED_BASE 0xa3000000u
#define DEVICE_DOORBELL 0xa0001000u
#define WC_WORDS 32
#define ROUNDS 16

static inline uint32_t read_cycles(void)
{
    uint32_t value;
    __asm__ volatile ("csrr %0, mcycle" : "=r"(value));
    return value;
}

static uint32_t run_descriptor_writes(uintptr_t base)
{
    volatile uint32_t *descriptor = (volatile uint32_t *)base;
    volatile uint32_t *doorbell = (volatile uint32_t *)(uintptr_t)DEVICE_DOORBELL;
    const uint32_t start = read_cycles();
    for (uint32_t round = 0; round < ROUNDS; ++round) {
        for (uint32_t i = 0; i < WC_WORDS; ++i) {
            descriptor[i] = (round << 24) | i;
        }
        __asm__ volatile ("fence w,w" ::: "memory");
        *doorbell = round;
    }
    __asm__ volatile ("fence w,w" ::: "memory");
    return read_cycles() - start;
}

/* Descriptor construction followed by a strongly ordered doorbell write. */
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    run_descriptor_writes(ORDERED_BASE);
    run_descriptor_writes(WC_BASE);

    const uint32_t ordered = run_descriptor_writes(ORDERED_BASE);
    const uint32_t combined = run_descriptor_writes(WC_BASE);
    printf("store descriptor: ordered=%u combined=%u speedup=%u.%02ux\n",
        ordered, combined, ordered / combined,
        (ordered % combined) * 100u / combined);
    return 0;
}
