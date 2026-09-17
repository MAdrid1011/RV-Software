RAM_ORIGIN = 0x80000000;
RAM_LENGTH = 0x08000000;
TEST_BASE = RAM_ORIGIN;
TOHOST_ORIGIN = 0xaffff000;
NUM_HARTS = 1;
STACK_SIZE = 0x20000;

OUTPUT_ARCH("riscv")
ENTRY(rvtest_entry_point)

MEMORY
{
    ram (rwx) : ORIGIN = RAM_ORIGIN, LENGTH = RAM_LENGTH
    io  (rw)  : ORIGIN = TOHOST_ORIGIN, LENGTH = 0x1000
}

PROVIDE(__stack_size = STACK_SIZE);
PROVIDE(__num_harts = NUM_HARTS);

SECTIONS
{
    .text.init TEST_BASE : { *(.text.init) } > ram
    .text.rvtest . : { *(.text.rvtest) *(.text.rvtest.*) } > ram

    . = ALIGN(0x4000);
    .rodata . : { *(.rodata) *(.rodata.*) *(.srodata) *(.srodata.*) } > ram
    .data . : { *(.data) *(.data.*) *(.sdata) *(.sdata.*) } > ram

    . = ALIGN(16);
    .bss . : {
        __bss_start = .;
        *(.sbss) *(.sbss.*) *(.bss) *(.bss.*) *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    } > ram

    . = ALIGN(16);
    __stack_bottom = .;
    . += __stack_size * __num_harts;
    __stack_top = .;

    . = ALIGN(0x1000);
    .text.rvmodel . : { *(.text.rvmodel) *(.text.rvmodel.*) *(.text) *(.text.*) } > ram
    . = ALIGN(0x1000);
    _end = .;

    .tohost TOHOST_ORIGIN : { KEEP(*(.tohost)) } > io

    ASSERT(_end <= ORIGIN(ram) + LENGTH(ram), "ACT ELF exceeds Zircon RAM")
}
