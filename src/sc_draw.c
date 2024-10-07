/*
 * sc_draw.c
 *
 * Copyright (C) 2023 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define SC_H_INTERNAL
#include "sc_base.h"


void
sc_draw_clear(
		sc_draw_t * s)
{
	for (int i = 0; i < s->line.count; i++)
		sc_line_free(&s->line.e[i]);
	sc_lines_free(&s->line);
	s->style.raw = 0;
	s->c_x = s->c_y = 0;
}

void
sc_draw_dirty(
		sc_draw_t * s)
{
	s->dirty = 1;
}

void
sc_draw_goto(
	sc_draw_t *s,
	int x, int y)
{
	if (x >= 0)
		s->c_x = x;
	if (y >= 0)
		s->c_y = y;
}

void
sc_draw_dispose(
		sc_draw_t *s)
{
	for (int li = 0; li < s->line.count; li++)
		sc_line_free(&s->line.e[li]);
	sc_lines_free(&s->line);
}

int
sc_draw_vprintf(
	sc_add_context_t *add,
	sc_draw_t *s,
	const char *f,
	va_list ap)
{
	sc_buf_t b = {};
	int res = sc_buf_vprintf(&b, f, ap);
	sc_add_context_t z = {};
	if (!add) add = &z;
	res = sc_draw_add(add, s, (char*)b.e, b.count);
	sc_buf_free(&b);
	return res;
}

int
sc_draw_printf(
	sc_add_context_t *add,
	sc_draw_t *s,
	const char *f,
	...)
{
	va_list ap;
	va_start(ap, f);
	int res = sc_draw_vprintf(add, s, f, ap);
	va_end(ap);
	return res;
}
