ENTRY(_start)
MEMORY { RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M }
SECTIONS {
    .text : { KEEP(*(.text.entry)) *(.text .text.*) } > RAM
    .rodata : { *(.rodata .rodata.*) } > RAM
    .data : {
        __global_pointer$ = . + 0x800;
        *(.data .data.* .sdata .sdata.*)
    } > RAM
    .bss (NOLOAD) : {
        . = ALIGN(32);
        __bss_start = .;
        *(.bss .bss.* .sbss .sbss.*) *(COMMON)
        . = ALIGN(32);
        __bss_end = .;
    } > RAM
    .stack (NOLOAD) : { . = ALIGN(16); . += 256K; __stack_top = .; } > RAM
    __heap_start = .;
    __heap_end = ORIGIN(RAM) + LENGTH(RAM);
    tohost = 0xaffff000;
    fromhost = 0xaffff004;
}
