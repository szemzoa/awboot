/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2012, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "common.h"
#include "string.h"
#include "fdt.h"
#include "debug.h"

#define OF_DT_TOKEN_NODE_BEGIN 0x00000001 /* Start node */
#define OF_DT_TOKEN_NODE_END   0x00000002 /* End node */
#define OF_DT_TOKEN_PROP	   0x00000003 /* Property */
#define OF_DT_TOKEN_NOP		   0x00000004
#define OF_DT_END			   0x00000009

static void _memcpy(void *dst, const void *src, size_t len)
{
	unsigned char		*cdst = dst;
	const unsigned char *csrc = src;
	while (len--)
		*cdst++ = *csrc++;
}

static inline unsigned int of_get_magic_number(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->magic_number);
}

static inline unsigned int of_get_format_version(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->format_version);
}

static inline unsigned int of_get_offset_dt_strings(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->offset_dt_strings);
}

static inline void of_set_offset_dt_strings(void *blob, unsigned int offset)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	header->offset_dt_strings = swap_uint32(offset);
}

static inline char *of_get_string_by_offset(void *blob, unsigned int offset)
{
	return (char *)((unsigned int)blob + of_get_offset_dt_strings(blob) + offset);
}

static inline unsigned int of_get_offset_dt_struct(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->offset_dt_struct);
}

static inline unsigned int of_dt_struct_offset(void *blob, unsigned int offset)
{
	return (unsigned int)blob + of_get_offset_dt_struct(blob) + offset;
}

unsigned int fdt_get_total_size(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->total_size);
}

static inline void of_set_dt_total_size(void *blob, unsigned int size)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	header->total_size = swap_uint32(size);
}

static inline unsigned int of_get_dt_strings_len(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->dt_strings_len);
}

static inline void of_set_dt_strings_len(void *blob, unsigned int len)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	header->dt_strings_len = swap_uint32(len);
}

static inline unsigned int of_get_dt_struct_len(void *blob)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	return swap_uint32(header->dt_struct_len);
}

static inline void of_set_dt_struct_len(void *blob, unsigned int len)
{
	boot_param_header_t *header = (boot_param_header_t *)blob;

	header->dt_struct_len = swap_uint32(len);
}

static inline int of_blob_data_size(void *blob)
{
	return (unsigned int)of_get_offset_dt_strings(blob) + of_get_dt_strings_len(blob);
}

/* -------------------------------------------------------- */

/* return the token and the next token offset
 */
static int of_get_token_nextoffset(void *blob, int startoffset, int *nextoffset, unsigned int *token)
{
	const unsigned int *p, *plen;
	unsigned int		tag;
	const char		   *cell;
	unsigned int		offset = startoffset;

	*nextoffset = -1;

	if (offset % 4) {
		debug("DT: the token offset is not aligned\r\n");
		return -1;
	}

	/* Get the token */
	p	= (unsigned int *)of_dt_struct_offset(blob, offset);
	tag = swap_uint32(*p);

	/* to get offset for the next token */
	offset += 4;
	if (tag == OF_DT_TOKEN_NODE_BEGIN) {
		/* node name */
		cell = (char *)of_dt_struct_offset(blob, offset);
		do {
			cell++;
			offset++;
		} while (*cell != '\0');
		/* the \0 is part of the node name, hence offset must be updated to the
		 * position past the \0.
		 */
		++offset;
	} else if (tag == OF_DT_TOKEN_PROP) {
		/* the property value size */
		plen = (unsigned int *)of_dt_struct_offset(blob, offset);
		/* name offset + value size + value */
		offset += swap_uint32(*plen) + 8;
	} else if ((tag != OF_DT_TOKEN_NODE_END) && (tag != OF_DT_TOKEN_NOP) && (tag != OF_DT_END))
		return -1;

	*nextoffset = OF_ALIGN(offset);
	*token		= tag;

	return 0;
}

static int of_get_nextnode_offset(void *blob, int start_offset, int *offset, int *nextoffset, int *depth)
{
	int			 next_offset = 0;
	int			 nodeoffset	 = start_offset;
	unsigned int token;
	int			 ret;

	if (!offset || !nextoffset || !depth)
		return -1;

	while (1) {
		ret = of_get_token_nextoffset(blob, nodeoffset, &next_offset, &token);
		if (ret)
			return ret;

		if (token == OF_DT_TOKEN_NODE_BEGIN) {
			/* find the node start token */
			(*depth)++;

			break;
		} else {
			nodeoffset = next_offset;

			if ((token == OF_DT_TOKEN_PROP) || (token == OF_DT_TOKEN_NOP))
				continue;
			else if (token == OF_DT_TOKEN_NODE_END) {
				(*depth)--;

				if ((*depth) < 0)
					return -1; /* not found */
			} else if (token == OF_DT_END)
				return -1; /* not found*/
		}
	};

	*offset		= nodeoffset;
	*nextoffset = next_offset;

	return 0;
}

static int of_get_node_offset(void *blob, const char *name, int *offset)
{
	int			 start_offset = 0;
	int			 nodeoffset	  = 0;
	int			 nextoffset	  = 0;
	int			 depth		  = 0;
	unsigned int token;
	unsigned int namelen = strlen(name);
	char		*nodename;
	int			 ret;

	/* find the root node*/
	ret = of_get_token_nextoffset(blob, 0, &start_offset, &token);
	if (ret)
		return -1;

	while (1) {
		ret = of_get_nextnode_offset(blob, start_offset, &nodeoffset, &nextoffset, &depth);
		if (ret)
			return ret;

		if (depth < 0)
			return -1;

		nodename = (char *)of_dt_struct_offset(blob, (nodeoffset + 4));
		if ((memcmp(nodename, name, namelen) == 0) && ((nodename[namelen] == '\0') || (nodename[namelen] == '@')))
			break;

		start_offset = nextoffset;
	}

	*offset = nextoffset;

	return 0;
}

/* -------------------------------------------------------- */

static int of_blob_move_dt_struct(void *blob, void *point, int oldlen, int newlen)
{
	void		*dest = point + newlen;
	void		*src  = point + oldlen;
	unsigned int len  = (char *)blob + of_blob_data_size(blob) - (char *)point - oldlen;

	int			 delta		   = newlen - oldlen;
	unsigned int structlen	   = of_get_dt_struct_len(blob) + delta;
	unsigned int stringsoffset = of_get_offset_dt_strings(blob) + delta;

	memmove(dest, src, len);

	of_set_dt_struct_len(blob, structlen);
	of_set_offset_dt_strings(blob, stringsoffset);

	if (delta > 0)
		of_set_dt_total_size(blob, fdt_get_total_size(blob) + delta);

	return 0;
}

static int of_blob_move_dt_string(void *blob, int newlen)
{
	void *point = (void *)((unsigned int)blob + of_get_offset_dt_strings(blob) + of_get_dt_strings_len(blob));

	void		*dest		= point + newlen;
	unsigned int len		= (char *)blob + of_blob_data_size(blob) - (char *)point;
	unsigned int stringslen = of_get_dt_strings_len(blob) + newlen;

	memmove(dest, point, len);

	of_set_dt_strings_len(blob, stringslen);
	of_set_dt_total_size(blob, fdt_get_total_size(blob) + newlen);

	return 0;
}

static int of_get_next_property_offset(void *blob, int startoffset, int *offset, int *nextproperty)
{
	unsigned int token;
	int			 nextoffset;
	int			 ret = -1;

	while (1) {
		ret = of_get_token_nextoffset(blob, startoffset, &nextoffset, &token);
		if (ret)
			break;

		if (token == OF_DT_TOKEN_PROP) {
			*offset		  = startoffset;
			*nextproperty = nextoffset;
			ret			  = 0;
			break;
		} else if (token == OF_DT_TOKEN_NOP)
			continue;
		else {
			ret = -1;
			break;
		}

		startoffset = nextoffset;
	};

	return ret;
}

static int of_get_property_offset_by_name(void *blob, unsigned int nodeoffset, const char *name, int *offset)
{
	unsigned int  nameoffset;
	unsigned int *p;
	unsigned int  namelen		  = strlen(name);
	int			  startoffset	  = nodeoffset;
	int			  property_offset = 0;
	int			  nextoffset	  = 0;
	char		 *string;
	int			  ret;

	*offset = 0;

	while (1) {
		ret = of_get_next_property_offset(blob, startoffset, &property_offset, &nextoffset);
		if (ret)
			return ret;

		p		   = (unsigned int *)of_dt_struct_offset(blob, property_offset + 8);
		nameoffset = swap_uint32(*p);
		string	   = of_get_string_by_offset(blob, nameoffset);
		if ((strlen(string) == namelen) && (memcmp(string, name, namelen) == 0)) {
			*offset = property_offset;
			return 0;
		}
		startoffset = nextoffset;
	}

	return -1;
}

static int of_string_is_find_strings_blob(void *blob, const char *string, int *offset)
{
	char *dt_strings	= (char *)blob + of_get_offset_dt_strings(blob);
	int	  dt_stringslen = of_get_dt_strings_len(blob);
	int	  len			= strlen(string) + 1;
	char *lastpoint		= dt_strings + dt_stringslen - len;
	char *p;

	for (p = dt_strings; p <= lastpoint; p++) {
		if (memcmp(p, string, len) == 0) {
			*offset = p - dt_strings;
			return 0;
		}
	}

	return -1;
}

static int of_add_string_strings_blob(void *blob, const char *string, int *name_offset)
{
	char *dt_strings	= (char *)blob + of_get_offset_dt_strings(blob);
	int	  dt_stringslen = of_get_dt_strings_len(blob);
	char *new_string;
	int	  len = strlen(string) + 1;
	int	  ret;

	new_string = dt_strings + dt_stringslen;
	ret		   = of_blob_move_dt_string(blob, len);
	if (ret)
		return ret;

	_memcpy(new_string, string, len);

	*name_offset = new_string - dt_strings;

	return 0;
}

static int of_add_property(void *blob, int nextoffset, const char *property_name, const void *value, int valuelen)
{
	int			  string_offset;
	unsigned int *p;
	unsigned int  addr;
	int			  len;
	int			  ret;

	/* check if the property name in the dt_strings,
	 * else add the string in dt strings
	 */
	ret = of_string_is_find_strings_blob(blob, property_name, &string_offset);
	if (ret) {
		ret = of_add_string_strings_blob(blob, property_name, &string_offset);
		if (ret)
			return ret;
	}

	/* add the property node in dt struct */
	len	 = 12 + OF_ALIGN(valuelen);
	addr = of_dt_struct_offset(blob, nextoffset);
	ret	 = of_blob_move_dt_struct(blob, (void *)addr, 0, len);
	if (ret)
		return ret;

	p = (unsigned int *)addr;

	/* set property node: token, value size, name offset, value */
	*p++ = swap_uint32(OF_DT_TOKEN_PROP);
	*p++ = swap_uint32(valuelen);
	*p++ = swap_uint32(string_offset);
	_memcpy((unsigned char *)p, value, valuelen);

	return 0;
}

static int of_update_property_value(void *blob, int property_offset, const void *value, int valuelen)
{
	int			   oldlen;
	unsigned int  *plen;
	unsigned char *pvalue;
	void		  *point;
	int			   ret;

	plen   = (unsigned int *)of_dt_struct_offset(blob, property_offset + 4);
	pvalue = (unsigned char *)of_dt_struct_offset(blob, property_offset + 12);
	point  = (void *)pvalue;

	/* get the old len of value */
	oldlen = swap_uint32(*plen);

	ret = of_blob_move_dt_struct(blob, point, OF_ALIGN(oldlen), OF_ALIGN(valuelen));
	if (ret)
		return ret;

	/* set the new len and value */
	*plen = swap_uint32(valuelen);

	_memcpy(pvalue, value, valuelen);

	return 0;
}

static int of_set_property(void *blob, int nodeoffset, const char *property_name, void *value, int valuelen)
{
	int property_offset;
	int ret;

	/* If find the property name in the dt blob, update its value,
	 * else to add this property
	 */
	ret = of_get_property_offset_by_name(blob, nodeoffset, property_name, &property_offset);
	if (ret) {
		trace("DT: adding property %s size %u\r\n", property_name, valuelen);
		ret = of_add_property(blob, nodeoffset, property_name, value, valuelen);
		if (ret)
			warning("DT: fail to add property\r\n");

		return ret;
	}

	ret = of_update_property_value(blob, property_offset, value, valuelen);
	if (ret) {
		warning("DT: fail to update property\r\n");
		return ret;
	}
	trace("DT: updated property %s size %u\r\n", property_name, valuelen);

	return 0;
}

static unsigned int of_get_property_cells(void *blob, int nodeoffset, const char *property_name)
{
	int			 property_offset;
	unsigned int	len;
	unsigned int	cells = (sizeof(uintptr_t) > 4U) ? 2U : 1U;

	if (of_get_property_offset_by_name(blob, nodeoffset, property_name, &property_offset) != 0) {
		return cells;
	}

	unsigned int *plen = (unsigned int *)of_dt_struct_offset(blob, property_offset + 4);
	len = swap_uint32(*plen);

	if (len == 8U) {
		return 2U;
	}
	if (len == 4U) {
		return 1U;
	}

	return cells;
}

static void of_encode_cells_be(uint8_t *dst, unsigned int cells, uint64_t value)
{
	if (cells > 1U) {
		uint32_t hi = (uint32_t)(value >> 32);
		uint32_t lo = (uint32_t)(value & 0xffffffffU);
		hi = swap_uint32(hi);
		lo = swap_uint32(lo);
		_memcpy(dst, &hi, sizeof(hi));
		_memcpy(dst + sizeof(hi), &lo, sizeof(lo));
	} else {
		uint32_t val32 = (uint32_t)(value & 0xffffffffU);
		val32 = swap_uint32(val32);
		_memcpy(dst, &val32, sizeof(val32));
	}
}

/* ---------------------------------------------------- */

int fdt_check_blob_valid(void *blob)
{
	return ((of_get_magic_number(blob) == OF_DT_MAGIC) && (of_get_format_version(blob) >= 17)) ? 0 : 1;
}

/* The /chosen node
 * property "bootargs": This zero-terminated string is passed
 * as the kernel command line.
 */
int fdt_update_bootargs(void *blob, const char *bootargs)
{
	int nodeoffset;
	int ret;

	ret = of_get_node_offset(blob, "chosen", &nodeoffset);
	if (ret) {
		warning("DT: doesn't support add node (chosen)\r\n");
		return ret;
	}

	/*
	 * if the property doesn't exit, add it
	 * if the property exists, update it.
	 */
	ret = of_set_property(blob, nodeoffset, "bootargs", (void *)bootargs, strlen(bootargs));
	if (ret) {
		warning("fail to set bootargs property\r\n");
		return ret;
	}

	return 0;
}

/* The /chosen node
 * property "linux,initrd-start" and "linux,initrd-end"
 */
int fdt_update_initrd(void *blob, uint32_t start, uint32_t end)
{
	int			 nodeoffset;
	int			 ret;
	unsigned int start_cells;
	unsigned int end_cells;
	uint8_t	 start_buf[8];
	uint8_t	 end_buf[8];

	if (start == 0U || end == 0U || end <= start) {
		warning("DT: invalid initrd range start=0x%08" PRIx32 " end=0x%08" PRIx32 "\r\n", start, end);
		return -1;
	}

	ret = of_get_node_offset(blob, "chosen", &nodeoffset);
	if (ret) {
		debug("DT: doesn't support add node (chosen)\r\n");
		return ret;
	}

	start_cells = of_get_property_cells(blob, nodeoffset, "linux,initrd-start");
	end_cells   = of_get_property_cells(blob, nodeoffset, "linux,initrd-end");

	if (start_cells > 2U) {
		start_cells = 2U;
	}
	if (end_cells > 2U) {
		end_cells = 2U;
	}

	of_encode_cells_be(start_buf, start_cells, (uint64_t)start);
	of_encode_cells_be(end_buf, end_cells, (uint64_t)end);

	ret = of_set_property(blob, nodeoffset, "linux,initrd-start", start_buf,
				  (int)(start_cells * sizeof(uint32_t)));
	if (ret) {
		warning("DT: could not set linux,initrd-start property\r\n");
		return ret;
	}

	ret = of_set_property(blob, nodeoffset, "linux,initrd-end", end_buf,
				  (int)(end_cells * sizeof(uint32_t)));
	if (ret) {
		warning("DT: could not set linux,initrd-end property\r\n");
		return ret;
	}

	return 0;
}

/* The /memory node
 * Required properties:
 * - device_type: has to be "memory".
 * - reg: this property contains all the physical memory ranges of your boards.
 */
int fdt_update_memory(void *blob, uint32_t mem_bank, uint32_t mem_size)
{
	int			 nodeoffset;
	unsigned int data[4];
	int			 valuelen;
	int			 ret;

	ret = of_get_node_offset(blob, "memory", &nodeoffset);
	if (ret) {
		warning("DT: doesn't support add node (memory)\r\n");
		return ret;
	}

	/*
	 * if the property doesn't exit, add it
	 * if the property exists, update it.
	 */
	/* set "device_type" property */
	ret = of_set_property(blob, nodeoffset, "device_type", "memory", sizeof("memory"));
	if (ret) {
		warning("DT: could not set device_type property\r\n");
		return ret;
	}

	/* set "reg" property */
	data[0]	 = swap_uint32(mem_bank);
	data[1]	 = swap_uint32(mem_size);
	valuelen = 8;
	ret		 = of_set_property(blob, nodeoffset, "reg", data, valuelen);
	if (ret) {
		warning("DT: could not set reg property\r\n");
		return ret;
	}

	return 0;
}
