// Copyright (C) 2006 Microchip Technology Inc. and its subsidiaries
//
// SPDX-License-Identifier: MIT

#include "string.h"
#include "main.h"

void *memset(void *dst, int val, unsigned long cnt)
{
	char *d = (char *)dst;

	while (cnt--)
		*d++ = (char)val;

	return dst;
}

int memcmp(const void *dst, const void *src, unsigned long cnt)
{
	const char *d = (const char *)dst;
	const char *s = (const char *)src;
	int			r = 0;

	while (cnt-- && (r = *d++ - *s++) == 0)
		;

	return r;
}

unsigned long strlen(const char *str)
{
	long i = 0;

	while (str[i++] != '\0')
		;

	return i - 1;
}

char *strcpy(char *dst, const char *src)
{
	char *bak = dst;

	while ((*dst++ = *src++) != '\0')
		;

	return bak;
}

char *strcat(char *dst, const char *src)
{
	char *p = dst;

	while (*dst != '\0')
		dst++;

	while ((*dst++ = *src++) != '\0')
		;

	return p;
}

int strcmp(const char *p1, const char *p2)
{
	unsigned char c1, c2;

	while (1) {
		c1 = *p1++;
		c2 = *p2++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
	}

	return 0;
}

int strncmp(const char *p1, const char *p2, unsigned long cnt)
{
	unsigned char c1, c2;

	while (cnt--) {
		c1 = *p1++;
		c2 = *p2++;

		if (c1 != c2)
			return c1 < c2 ? -1 : 1;

		if (!c1)
			break;
	}

	return 0;
}

char *strchr(const char *s, int c)
{
	for (; *s != (char)c; ++s)
		if (*s == '\0')
			return NULL;

	return (char *)s;
}

/* NOTE: This is the simple-minded O(len(s1) * len(s2)) worst-case approach. */

char *strstr(const char *s1, const char *s2)
{
	register const char *s = s1;
	register const char *p = s2;

	do {
		if (!*p) {
			return (char *)s1;
			;
		}
		if (*p == *s) {
			++p;
			++s;
		} else {
			p = s2;
			if (!*s) {
				return NULL;
			}
			s = ++s1;
		}
	} while (1);
}

void *memchr(const void *src, int val, unsigned long cnt)
{
	char *p = NULL;
	char *s = (char *)src;

	while (cnt) {
		if (*s == val) {
			p = s;
			break;
		}
		s++;
		cnt--;
	}

	return p;
}

void *memmove(void *dst, const void *src, unsigned long cnt)
{
	char *p, *s;

	if (dst <= src) {
		p = (char *)dst;
		s = (char *)src;
		while (cnt--)
			*p++ = *s++;
	} else {
		p = (char *)dst + cnt;
		s = (char *)src + cnt;
		while (cnt--)
			*--p = *--s;
	}

	return dst;
}

void *memcpy(void *dst, const void *src, unsigned long cnt)
{
	char		 *d;
	const char *s;
	struct chunk {
		unsigned long val[2];
	};

	const struct chunk *csrc = (const struct chunk *)src;
	struct chunk		 *cdst = (struct chunk *)dst;

	if (((unsigned long)src & 0xf) == 0 && ((unsigned long)dst & 0xf) == 0) {
		while (cnt >= sizeof(struct chunk)) {
			*cdst++ = *csrc++;
			cnt -= sizeof(struct chunk);
		}
	}

	d = (char *)cdst;
	s = (const char *)csrc;

	while (cnt--)
		*d++ = *s++;

	return dst;
}
