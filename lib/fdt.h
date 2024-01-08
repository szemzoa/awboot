/*
 * Copyright (C) 2012 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __FDT_H__
#define __FDT_H__

#define OF_DT_MAGIC 0xd00dfeed

typedef struct {
	unsigned int magic_number;
	unsigned int total_size;
	unsigned int offset_dt_struct;
	unsigned int offset_dt_strings;
	unsigned int offset_reserve_map;
	unsigned int format_version;
	unsigned int last_compatible_version;

	/* version 2 field */
	unsigned int mach_id;
	/* version 3 field */
	unsigned int dt_strings_len;
	/* version 17 field */
	unsigned int dt_struct_len;
} boot_param_header_t;

extern unsigned int of_get_dt_total_size(void *blob);
extern unsigned int of_get_magic_number(void *blob);
extern int			check_dt_blob_valid(void *blob);
extern int			fixup_chosen_node(void *blob, char *bootargs);
extern int fixup_memory_node(void *blob, unsigned int *mem_bank, unsigned int *mem_bank2, unsigned int *mem_size);
#endif /* #ifndef __FDT_H__ */
