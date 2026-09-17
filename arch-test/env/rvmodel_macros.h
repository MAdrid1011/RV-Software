#ifndef ZIRCON_RVMODEL_MACROS_H
#define ZIRCON_RVMODEL_MACROS_H

#define RVMODEL_DATA_SECTION                         \
    .pushsection .tohost, "aw", @progbits;           \
    .balign 8; .global tohost; tohost: .dword 0;     \
    .balign 8; .global fromhost; fromhost: .dword 0; \
    .popsection;

#define STANDARD_SM_SUPPORTED
#define SAIL_CLINT_BASE_ADDRESS 0x02000000
#define SAIL_SIMPLE_INTERRUPT_GENERATOR_BASE_ADDRESS 0x10000000

#define RVMODEL_HALT_PASS \
    li x1, 1;             \
    la t0, tohost;        \
    sw x1, 0(t0);         \
1:  j 1b;

#define RVMODEL_HALT_FAIL \
    li x1, 3;             \
    la t0, tohost;        \
    sw x1, 0(t0);         \
1:  j 1b;

#define RVMODEL_IO_INIT(_R1, _R2, _R3)
#define RVMODEL_IO_WRITE_STR(_R1, _R2, _R3, _STR_PTR)
#define RVMODEL_ACCESS_FAULT_ADDRESS 0x00000000
#define RVMODEL_INTERRUPT_LATENCY 16
#define RVMODEL_TIMER_INT_SOON_DELAY 64
#define RVMODEL_SET_MEXT_INT(_R1, _R2)
#define RVMODEL_CLR_MEXT_INT(_R1, _R2)
#define RVMODEL_MSIP_ADDRESS 0xa0001000

#endif
