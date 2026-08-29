ENTRY(_start)

SECTIONS {
    . = 0x8000000000;
    
    .text : ALIGN(4K) {
        *(.text .text.* .ltext .ltext.*)
    }
    
    .rodata : ALIGN(4K) {
        *(.rodata .rodata.* .lrodata .lrodata.*)
    }
    
    .data : ALIGN(4K) {
        *(.data .data.* .ldata .ldata.*)
    }
    
    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss .bss.* .lbss .lbss.*)
    }
}
