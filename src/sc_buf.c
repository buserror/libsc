/*
 * sc_buf.c
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SC_H_INTERNAL
#include "sc_base.h"

int
sc_buf_concat(
	sc_buf_t *b,
	uint8_t *e,
	unsigned int c)
{
	if (!b || !e) return -1;
	if (!c) c = strlen((char*)e);
	return sc_buf_insert(b, b->count, e, c);
}

int
sc_buf_vprintf(
	sc_buf_t *b,
	const char *f,
	va_list ap)
{
	if (!b || !f) return -1;
	va_list c;
	va_copy(c, ap);
	int l = vsnprintf(NULL, 0, f, c);
	if ((b->count + l) >= b->size)
		sc_buf_realloc(b, b->count + l + 1);
	va_end(c);
	/*int dl = */vsnprintf((char*)b->e + b->count, l + 1, f, ap);
	b->count += l;
	return l;
}

int
sc_buf_printf(
	sc_buf_t *b,
	const char *f,
	...)
{
	if (!b || !f) return -1;
	va_list ap;
	va_start(ap, f);
	int res = sc_buf_vprintf(b, f, ap);
	va_end(ap);
	return res;
}

