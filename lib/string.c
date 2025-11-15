// Copyright (C) 2006 Microchip Technology Inc. and its subsidiaries
//
// SPDX-License-Identifier: MIT

#include <limits.h>
#include "string.h"
#include "common.h"

void *memset(void *dst, int val, size_t len)
{
	char *d = (char *)dst;

	while (len--)
		*d++ = (char)val;

	return dst;
}

int memcmp(const void *dst, const void *src, size_t len)
{
	const char *d = (const char *)dst;
	const char *s = (const char *)src;
	int			r = 0;

	while (len-- && (r = *d++ - *s++) == 0)
		;

	return r;
}

size_t strlen(const char *str)
{
	int i = 0;

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

char *strncpy(char *dst, const char *src, size_t len)
{
	char *bak = dst;
	if (!len)
		return bak;

	while ((*dst++ = *src++) != '\0' && len--)
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

int strncmp(const char *p1, const char *p2, size_t len)
{
	unsigned char c1, c2;

	while (len--) {
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

void *memchr(void *src, int val, size_t len)
{
	char *p = NULL;
	char *s = (char *)src;

	while (len) {
		if (*s == val) {
			p = s;
			break;
		}
		s++;
		len--;
	}

	return p;
}

void *memmove(void *dst, const void *src, size_t len)
{
	char *p, *s;

	if (dst <= src) {
		p = (char *)dst;
		s = (char *)src;
		while (len--)
			*p++ = *s++;
	} else {
		p = (char *)dst + len;
		s = (char *)src + len;
		while (len--)
			*--p = *--s;
	}

	return dst;
}

int atoi(char *str)
{
	int sign = 1, base = 0, i = 0;

	// if whitespaces then ignore.
	while (str[i] == ' ') {
		i++;
	}

	// sign of number
	if (str[i] == '-' || str[i] == '+') {
		sign = 1 - 2 * (str[i++] == '-');
	}

	// checking for valid input
	while (str[i] >= '0' && str[i] <= '9') {
		// handling overflow test case
		if (base > INT_MAX / 10 || (base == INT_MAX / 10 && str[i] - '0' > 7)) {
			if (sign == 1)
				return INT_MAX;
			else
				return INT_MIN;
		}
		base = 10 * base + (str[i++] - '0');
	}
	return base * sign;
}
