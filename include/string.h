/*
 * Copyright (C) 2006 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __STRING_H__
#define __STRING_H__

#include "types.h"

void *memcpy(void *dst, const void *src, int cnt);
void *memset(void *dst, int val, int cnt);
int memcmp(const void *dst, const void *src, unsigned int cnt);
unsigned int strlen(const char *str);
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);
int strcmp(const char *p1, const char *p2);
int strncmp(const char *p1, const char *p2, unsigned int cnt);
int strspn(const char *s1, const char *s2);
int strcspn(const char *s1, const char *s2);
char *strchr(const char *s, int c);
char *strstr(const char *s, const char *what);
void *memchr(void *ptr, int value, unsigned int num);
void *memmove(void *dest, const void *src, unsigned int count);

#endif /* #ifndef __STRING_H__ */
