/*
 * sc_store.c
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SC_H_INTERNAL
#include "sc_base.h"
#include "minipt.h"

enum {
	SC_ADD_NO_ADVANCE = (1 << 0),
};

sc_glyph_t *
sc_draw_store_xy(
	sc_draw_t *s,
	const sc_glyph_t *g,
	int x, int y)
{
	if (x < 0 || y < 0)
		return NULL;
	while (s->line.count <= y) {
		static const sc_line_t zero = {};
		sc_lines_add(&s->line, zero);
	}
	sc_line_t * l = &s->line.e[y];
	const sc_glyph_t zero = {};
	while (l->count <= x) {
		sc_line_add(l, zero);
	}
	if (g) {
		if (l->e[x].g != g->g || l->e[x].style.raw != g->style.raw) {
			sc_draw_dirty(s);
			l->e[x] = *g;
		}
	} else {
		if (l->e[x].g || l->e[x].style.raw) {
			sc_draw_dirty(s);
			l->e[x] = zero;
		}
	}
	return &l->e[x];
}

int
_sc_draw_store_add(
	sc_draw_t *s,
	uint32_t c,
	uint8_t flags)
{
	while (s->line.count <= s->c_y) {
		static const sc_line_t zero = {};
		sc_lines_add(&s->line, zero);
	}
	sc_line_t * l = &s->line.e[s->c_y];
	while (l->count <= s->c_x) {
		const sc_glyph_t zero = {};
		sc_line_add(l, zero);
	}
	sc_glyph_t g = { .g = c, .style = s->style };
	if (l->e[s->c_x].raw != g.raw) {
		l->e[s->c_x] = g;
		sc_draw_dirty(s);
	}
	if (!(flags & SC_ADD_NO_ADVANCE)) {
		s->c_x++;
		if (s->c_x >= SC_LINE_MAX) {
			s->c_x = 0;
			s->c_y++;
		}
	}
	return s->dirty;
}

static void
_sc_draw_store_csi(
	sc_add_context_t *add,
	sc_draw_t *s,
	uint8_t c)
{
	switch (c) {
		case 'A': { // move Y up
			int cl = 1;
			if (add->pcount)
				cl = add->p[0];
			if (s->c_y > cl)
				s->c_y = 0;
			else
				s->c_y -= cl;
		}	break;
		case 'G': { // Move absolute X
			int cl = 1;
			if (add->pcount)
				cl = add->p[0];
			if (cl) cl--;	// 1 based to zero based
			s->c_x = cl;
		}	break;
		case 'm': {	// style/colors
			for (int pi = 0; pi < add->pcount; pi++) {
				//	printf("CSI %d m\n", add->p[pi]);
				switch (add->p[pi]) {
					case 0:
						s->style.raw = 0;
						break;
					case 1:
					case 21:
						s->style.bold = add->p[pi] < 10;
						break;
					case 4:
					case 24:
						s->style.under = add->p[pi] < 10;
						break;
					case 7:
					case 27:
						s->style.invert = add->p[pi] < 10;
						break;
					case 22:
						s->style.bold = 0;
						break;
					case 30 ... 37:
						s->style.fore = add->p[pi] - 30;
						s->style.has_fore = 1;
						break;
					case 38: {	// 256 colors
						if (add->pcount - pi < 3 || add->p[pi + 1] != 5)
							break;
						s->style.fore = add->p[pi + 2];
						s->style.has_fore = 1;
						pi += 3;
					}	break;
					case 39:
						s->style.fore = 0;
						s->style.has_fore = 0;
						break;
					case 40 ... 47:
						s->style.back = add->p[pi] - 40;
						s->style.has_back = 1;
						break;
					case 48: {	// 256 colors
						if (add->pcount - pi < 3 || add->p[pi + 1] != 5)
							break;
						s->style.back = add->p[pi + 2];
						s->style.has_back = 1;
						pi += 3;
					}	break;
					case 49:
						s->style.back = 0;
						s->style.has_back = 0;
						break;
				}
			}
		}	break;
	}
}

static void
_sc_draw_store_esc(
	sc_add_context_t *add,
	sc_draw_t *s,
	uint8_t c)
{
	switch (c) {
		case 'c': // reset all
			sc_draw_clear(s);
			break;
	}
}

static void
_sc_draw_store_param_digit(
	sc_add_context_t *add,
	uint8_t c)
{
	if (!add->pcount) {
		add->p[0] = 0;
		add->pcount = 1;
	}
	add->p[add->pcount - 1] *= 10;
	add->p[add->pcount - 1] += c - '0';
}

// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

static inline unsigned int
stb_ttc__UTF8_Decode(
		unsigned int* state,
		unsigned int* codep,
		unsigned char byte)
{
	static const unsigned char utf8d[] = {
		// The first part of the table maps bytes to character classes that
		// to reduce the size of the transition table and create bitmasks.
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

		// The second part is a transition table that maps a combination
		// of a state of the automaton and a character class to a state.
		0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
		12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
		12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
		12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
		12,36,12,12,12,12,12,12,12,12,12,12,
	};
	unsigned int type = utf8d[byte];
	*codep = (*state != UTF8_ACCEPT) ?
				(byte & 0x3fu) | (*codep << 6) :
				(0xff >> type) & (byte);
	*state = utf8d[256 + *state + type];
	return *state;
}

static void
_sc_draw_store_machine(
	sc_add_context_t *add,
	sc_draw_t *s,
	uint8_t c)
{
	pt_start(add->pt);

	switch (c) {
		case 27: {
			do {
				pt_yield(add->pt);
				switch (c) {
					case '[': {
						add->pcount = 0;
						do {
							pt_yield(add->pt);
							switch (c) {
								case '0' ... '9':
									_sc_draw_store_param_digit(add, c);
									break;
								case ';':
									add->pcount++;
									add->p[add->pcount - 1] = 0;
									break;
								default:
									_sc_draw_store_csi(add, s, c);
									pt_finish(add->pt);	// restart machine
							}
						} while (1);
					}	break;
					case '0' ... '9':
						_sc_draw_store_param_digit(add, c);
						break;
					default:
						_sc_draw_store_esc(add, s, c);
						pt_finish(add->pt);	// restart machine
				}
			} while (1);
		}	break;
		case '\t':
			s->c_x = (s->c_x + 1) & ~3;
			break;
		case '\b':
			if (s->c_x) s->c_x--;
			break;
		case '\n':
		case '\r': {
			sc_line_t * l = &s->line.e[s->c_y];
			if (l->count > s->c_x)	// trim line
				l->count = s->c_x;
			s->c_x = 0;
			if (c == '\n')
				s->c_y++;
		}	break;
		default:
			if (stb_ttc__UTF8_Decode(&add->utf8_state, &add->utf8_glyph, c) == UTF8_ACCEPT)
				_sc_draw_store_add(s, add->utf8_glyph, 0);
	}
	pt_end(add->pt);
}

//! like sc_add but using a add context and a sc_draw ('add' is optional)
int
sc_draw_add(
	sc_add_context_t *add,
	sc_draw_t *s,
	const char *what,
	unsigned int l)
{
	if (!what || !*what)
		return 0;
	if (!l) l = strlen(what);
	sc_add_context_t z = {};
	if (!add) add = &z;
	for (int ci = 0; ci < l; ci++)
		_sc_draw_store_machine(add, s, what[ci]);
	return l;
}

int
sc_add(
	sc_t *sc,
	const char *what,
	unsigned int l)
{
	if (!what || !*what)
		return 0;
	if (!l) l = strlen(what);
	SC_GET(sc);
	sc_draw_add(&sc->add, &sc->current->draw, what, l);
	if (sc->current->draw.dirty)
		sc_win_dirty(sc->current);
	return l;
}

int
sc_printf(
	sc_t *sc,
	const char *f,
	...)
{
	SC_GET(sc);
	va_list ap;
	va_start(ap, f);
	int res = sc_draw_vprintf(&sc->add,
					&sc->current->draw, f, ap);
	va_end(ap);
	if (sc->current->draw.dirty)
		sc_win_dirty(sc->current);
	return res;
}
