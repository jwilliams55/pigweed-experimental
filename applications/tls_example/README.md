# TLS Example Application

The folder hosts an example TLS application based on pigweed TLS modules. The
example application connects to www.google.com:443, performs hanshake, verifies
certificate, sends a https get request and listens for response. The
application can be built into multiple variants using different TLS libraries
and transport backends (impelmented in backends/). See BUILD.gn for examples.

Due to the use of TLS libraries, the binary size of the application can
potentially be very big (300K ~ 600K+). Make sure that the target board has
enough FLASH/RAM for running the application. For pigweed teensy41 target, it
is recommended to modify the linker script to run all code from FLASH instead
of ITCM. Specifically, in linker script
`arduino/cores/teensy/hardware/teensy/avr/cores/teensy4/imxrt1062_t41.ld`
make the following changes:

Move section *(.text*) from

    ...
    .text.itcm : {
        . = .  32; /* MPU to trap NULL pointer deref */
        *(.fastrun)
        *(.text*)
        . = ALIGN(16);
    } > ITCM  AT> FLASH
    ...

into segment `.text.progmem`. i.e.:

    ...
    .text.progmem : {
        KEEP(*(.flashconfig))
        FILL(0xFF)
        . = ORIGIN(FLASH)  0x1000;
        KEEP(*(.ivt))
        KEEP(*(.bootdata))
        KEEP(*(.vectors))
        KEEP(*(.startup))
        *(.text*)
        *(.flashmem*)
        *(.progmem*)
                . = ALIGN(4);
                KEEP(*(.init))
                __preinit_array_start = .;
                KEEP (*(.preinit_array))
                __preinit_array_end = .;
                __init_array_start = .;
                KEEP (*(.init_array))
                __init_array_end = .;
        . = ALIGN(16);
    } > FLASH
    ...

Place segment .ARM.exids in FLASH. i.e:

    ...
    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH
    ...

Before flashing the elf binary to the target board, consider stripping the
binary to remove debug and symbol sections to make transfer faster. i.e.

arm-none-eabi-strip -s input.elf -o output.elf
