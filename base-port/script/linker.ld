ENTRY(_start)

SECTIONS {
    /* _pmem_start and _entry_offset are defined in LDFLAGS */
    . = _pmem_start + _entry_offset;
    .text : {
        *(entry)
        *(.text*)
    }
    etext = .;
    _etext = .;
    .rodata : {
        *(.rodata*)
    }
    .data : {
        *(.data)
    }
    edata = .;
    _data = .;
    .bss : {
        _bss_start = .;
        *(.bss*)
        *(.sbss*)
        *(.scommon)
    }
    _stack_top = ALIGN(0x1000);
    . = _stack_top + 0x10000;
    _stack_pointer = .;
    end = .;
    _end = .;
    _heap_start = ALIGN(0x1000);

    /* Reserve the final page of the 0xAxxxxxxx device window for simulation I/O. */
    tohost = 0xaffff000;
    fromhost = 0xaffff004;
}
