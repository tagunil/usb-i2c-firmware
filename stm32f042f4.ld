/* Linker script for STM32F042F4, 16k flash, 6k RAM. */

/* Define memory regions. */
MEMORY
{
    rom (rx) : ORIGIN = 0x08000000, LENGTH = 16K
    ram (rwx) : ORIGIN = 0x20000000, LENGTH = 6K
}

/* Include the common ld script. */
INCLUDE cortex-m-generic.ld
