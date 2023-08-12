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
