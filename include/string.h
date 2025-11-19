/*
 * Copyright (C) 2006 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __STRING_H__
#define __STRING_H__

#include "types.h"
#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t len);
void *memset(void *dst, int val, size_t len);
int	  memcmp(const void *dst, const void *src, size_t len);
void *memchr(void *ptr, int value, size_t len);
void *memmove(void *dest, const void *src, size_t count);

size_t strlen(const char *str);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t len);
char  *strcat(char *dst, const char *src);
int	   strcmp(const char *p1, const char *p2);
int	   strncmp(const char *p1, const char *p2, size_t len);
char  *strchr(const char *s, int c);
char  *strstr(const char *s, const char *what);
int	   atoi(char *str);

#endif /* #ifndef __STRING_H__ */
