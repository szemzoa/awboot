#include <arch/arm32/include/linkage.h>

#define CONFIG_NBOOT_STACK	0x38000

#define  ARMV7_USR_MODE        0x10
#define  ARMV7_FIQ_MODE        0x11
#define  ARMV7_IRQ_MODE        0x12
#define  ARMV7_SVC_MODE        0x13
#define  ARMV7_MON_MODE        0x16
#define  ARMV7_ABT_MODE        0x17
#define  ARMV7_UND_MODE        0x1b
#define  ARMV7_SYSTEM_MODE     0x1f
#define  ARMV7_MODE_MASK       0x1f
#define  ARMV7_FIQ_MASK        0x40
#define  ARMV7_IRQ_MASK        0x80

.text
        .arm

	.globl	reset
/*
	.globl	save_boot_params_ret
	.type   save_boot_params_ret,%function
*/
reset:
        /* Boot head information for BROM */
        .long 0xea000016
        .byte 'e', 'G', 'O', 'N', '.', 'B', 'T', '0'
        .long 0x12345678                                /* checksum */
        .long __spl_size                                /* spl size */
        .long 0x30                                              /* boot header size */
        .long 0x30303033                                /* boot header version */
        .long 0x00020000                                /* return value */
        .long 0x00028000                                /* run address */
        .long 0x0                                               /* eGON version */
        .byte 0x00, 0x00, 0x00, 0x00    /* platform information - 8byte */
        .byte 0x34, 0x2e, 0x30, 0x00

/*
 * The actual reset code
 */
        mrs r0, cpsr
        bic r0, r0, #ARMV7_MODE_MASK
        orr r0, r0, #ARMV7_SVC_MODE
        orr r0, r0, #(ARMV7_IRQ_MASK | ARMV7_FIQ_MASK)
        bic r0, r0, #(1<<9)     @set little-endian
        msr cpsr_c, r0

        /* Set vector base address register */
/*
        ldr r0, =_vector
        mcr p15, 0, r0, c12, c0, 0
        mrc p15, 0, r0, c1, c0, 0
        bic r0, #(1 << 13)
        mcr p15, 0, r0, c1, c0, 0
*/

        mrc     p15, 0, r0, c1, c0, 0
        bic     r0, r0, #0x00002000     @ clear bits 13 (--V-)
        bic     r0, r0, #0x00000007     @ clear bits 2:0 (-CAM)
        orr     r0, r0, #0x00000800     @ set bit 11 (Z---) BTB
        bic     r0, r0, #0x00001000     @ clear bit 12 (I) I-cache
        mcr     p15, 0, r0, c1, c0, 0

/*        ldr sp, =CONFIG_NBOOT_STACK
*/
        bl  clear_bss
/*
	bl  cpu_init_cp15
*/

       /*
         * disable interrupts (FIQ and IRQ), also set the cpu to SVC32 mode,
         * except if in HYP mode already
         */
        mrs     r0, cpsr
        and     r1, r0, #0x1f           @ mask mode bits
        teq     r1, #0x1a               @ test for HYP mode
        bicne   r0, r0, #0x1f           @ clear all mode bits
        orrne   r0, r0, #0x13           @ set SVC mode
        orr     r0, r0, #0xc0           @ disable FIQ and IRQ
        msr     cpsr,r0


        @set cntfrq to 24M
        ldr r0, =24000000
        mcr p15, 0, r0, c14, c0, 0

        bl main

clear_bss:
        ldr     r0, =_sbss
        ldr     r1, =_ebss
        mov     r2, #0

clbss_1:
        stmia   r0!, {r2}
        cmp r0, r1
        blt clbss_1

        mov pc, lr
