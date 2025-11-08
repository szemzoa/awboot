/**
 * \file
 *
 * \brief Linker script for T113-S3 internal SRAM
 *
 * based on: Linker script for running in internal FLASH on the SAMD21G17D
 * Copyright (c) 2018 Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/* This file gets parsed by the preprocessor */
#include "include/config.h"

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/* Memory Spaces Definitions */
MEMORY
{
  ram   (rwx) : ORIGIN = __RAM_BASE, LENGTH = 128K /* A1 + DSP0 IRAM + DSP0 DRAM0. 160K in boot mode, 128K in FEL mode */
  ddr   (rwx) : ORIGIN = 0x40000000, LENGTH = 128M
}

/* The stack size used by the application. NOTE: you need to adjust according to your application. */
STACK_SIZE = 0x2000; /* 8KB */

ENTRY(reset)

/* Section Definitions */
SECTIONS
{

    .text :
    {
    . = ALIGN(4);
    PROVIDE(__spl_start = .);
    *(.text .text.*)
    . = ALIGN(4);
    } > ram

    .secure_text :
    {
    . = ALIGN(4);
    __secure_start = .;
    * (._secure.text)
    . = ALIGN(4);
    } > ram

    .secure_data :
    {
    * (._secure.data)
    __secure_end = .;
    } > ram

    . = ALIGN(4);

    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > ram

    PROVIDE(__spl_end = .);
    PROVIDE(__spl_size = __spl_end - __spl_start);

    /* .bss section which is used for uninitialized data */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = . ;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = . ;
    } > ram

    .psci_stack (NOLOAD) :
    {
        . = ALIGN(4);
        __secure_stack_start = .;
        . = . + (CONFIG_ARMV7_PSCI_NR_CPUS * (1 << 10));
        . = ALIGN(4);
        __secure_stack_end = .;
    } > ram

    .stack (NOLOAD):
    {
        . = ALIGN(8);
    /* SRV stack section */
        __stack_srv_start = .;
        . += STACK_SIZE;
        __stack_srv_end = .;
    } > ram

    . = ALIGN(4);
    _end = . ;
}
