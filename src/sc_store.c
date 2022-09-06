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
sc_store_xy(
	sc_win_t *s,
	const sc_glyph_t *g,
	int x, int y)
{
	while (s->line.count <= y) {
		static const sc_line_t zero = {};
		sc_lines_add(&s->line, zero);
	}
	sc_line_t * l = &s->line.e[y];
	while (l->count <= x) {
		const sc_glyph_t zero = {};
		sc_line_add(l, zero);
	}
	if (g)
		l->e[x] = *g;
	return &l->e[x];
}

int
_sc_add_store(
	sc_win_t *s,
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
		sc_win_dirty(s);
	}
	if (!(flags & SC_ADD_NO_ADVANCE)) {
		s->c_x++;
		if (s->c_x >= SC_LINE_MAX) {
			s->c_x = 0;
			s->c_y++;
		}
	}
	return 0;
}

static void
sc_store_do_csi(
	sc_t *sc,
	uint8_t c)
{
	sc_win_t *s = sc->current;
	switch (c) {
		case 'A': { // move Y up
			int c = 1;
			if (sc->add.pcount)
				c = sc->add.p[0];
			if (s->c_y > c)
				s->c_y = 0;
			else
				s->c_y -= c;
		}	break;
		case 'G': { // Move absolute X
			int c = 1;
			if (sc->add.pcount)
				c = sc->add.p[0];
			if (c) c--;	// 1 based to zero based
			s->c_x = c;
		}	break;
		case 'm': {	// style/colors
			for (int pi = 0; pi < sc->add.pcount; pi++) {
				//	printf("CSI %d m\n", sc->add.p[pi]);
				switch (sc->add.p[pi]) {
					case 0:
						s->style.raw = 0;
						break;
					case 1:
					case 21:
						s->style.bold = sc->add.p[pi] < 10;
						break;
					case 4:
					case 24:
						s->style.under = sc->add.p[pi] < 10;
						break;
					case 7:
					case 27:
						s->style.invert = sc->add.p[pi] < 10;
						break;
					case 22:
						s->style.bold = 0;
						break;
					case 30 ... 37:
						s->style.fore = sc->add.p[pi] - 30;
						s->style.has_fore = 1;
						break;
					case 38: {	// 256 colors
						if (sc->add.pcount - pi < 3 || sc->add.p[pi + 1] != 5)
							break;
						s->style.fore = sc->add.p[pi + 2];
						s->style.has_fore = 1;
						pi += 3;
					}	break;
					case 39:
						s->style.fore = 0;
						s->style.has_fore = 0;
						break;
					case 40 ... 47:
						s->style.back = sc->add.p[pi] - 40;
						s->style.has_back = 1;
						break;
					case 48: {	// 256 colors
						if (sc->add.pcount - pi < 3 || sc->add.p[pi + 1] != 5)
							break;
						s->style.back = sc->add.p[pi + 2];
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
sc_store_do_esc(
	sc_t *sc,
	uint8_t c)
{
	switch (c) {
		case 'c': // reset all
			sc_win_clear(sc->current);
			break;
	}
}

static void
sc_store_param_digit(
	sc_t *sc,
	uint8_t c)
{
	if (!sc->add.pcount) {
		sc->add.p[0] = 0;
		sc->add.pcount = 1;
	}
	sc->add.p[sc->add.pcount - 1] *= 10;
	sc->add.p[sc->add.pcount - 1] += c - '0';
}

static void
sc_add_machine(
	sc_t *sc,
	uint8_t c)
{
	sc_win_t *s = sc->current;
	pt_start(sc->add.pt);

	switch (c) {
		case 27: {
			do {
				pt_yield(sc->add.pt);
				switch (c) {
					case '[': {
						sc->add.pcount = 0;
						do {
							pt_yield(sc->add.pt);
							switch (c) {
								case '0' ... '9':
									sc_store_param_digit(sc, c);
									break;
								case ';':
									sc->add.pcount++;
									sc->add.p[sc->add.pcount - 1] = 0;
									break;
								default:
									sc_store_do_csi(sc, c);
									pt_end(sc->add.pt);	// restart machine
							}
						} while (1);
					}	break;
					case '0' ... '9':
						sc_store_param_digit(sc, c);
						break;
					default:
						sc_store_do_esc(sc, c);
						pt_end(sc->add.pt);	// restart machine
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
#if 0	// later
			if (c & 0x80) {
				sc->add.utf8 = 0;
				sc->add.utf8_count = 0;

			} else
#endif
			_sc_add_store(s, c, 0);
	}
	pt_end(sc->add.pt);
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

	for (int ci = 0; ci < l; ci++)
		sc_add_machine(sc, what[ci]);
	return l;
}

int
sc_printf(
	sc_t *sc,
	const char *f,
	...)
{
	va_list ap;
	sc_buf_t b = {};
	va_start(ap, f);
	int res = sc_buf_vprintf(&b, f, ap);
	va_end(ap);
	res = sc_add(sc, (char*)b.e, b.count);
	sc_buf_free(&b);
	return res;
}
