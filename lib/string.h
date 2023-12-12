/*
 * Copyright (C) 2006 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __STRING_H__
#define __STRING_H__

#include "types.h"

extern void		*memset(void *dst, int val, unsigned long cnt);
extern void		*memcpy(void *dst, const void *src, unsigned long cnt);
extern int			memcmp(const void *dst, const void *src, unsigned long cnt);
extern char		*strchr(const char *s, int c);
extern char		*strcpy(char *dst, const char *src);
extern unsigned long strlen(const char *str);
extern char		*strcat(char *dst, const char *src);
extern int			strcmp(const char *p1, const char *p2);
extern int			strncmp(const char *p1, const char *p2, unsigned long cnt);
extern char		*strstr(const char *s, const char *what);
extern void		*memchr(const void *ptr, int value, unsigned long num);
extern void		*memmove(void *dest, const void *src, unsigned long count);

#endif /* #ifndef __STRING_H__ */
