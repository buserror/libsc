/*
 * sc_render.c
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SC_H_INTERNAL
#include "sc_base.h"

static char *
_sc_render_utf8_glyph(
	char * dst,
	unsigned int uni_glyph )
{
	if (uni_glyph < 128) {
		*dst = (char)uni_glyph;
		return dst + 1;	// that was easy
	}
	unsigned char *cur = (unsigned char*)dst;

	int cnt = 0;
	unsigned int mask = ~0x3f;
	while (uni_glyph & mask) {
		mask <<= 6;
		cnt++;
	}
	if ((uni_glyph >> (cnt *6)) & (0xf80 >> (cnt+1)))
		cnt++;
	unsigned char h = 0xf80 >> cnt;
	unsigned char hd = uni_glyph >> (cnt * 6);
	*cur++ = h | hd;
	while (cnt > 0) {
		cnt--;
		*cur++ = 0x80 | ((unsigned char)(uni_glyph >> (cnt * 6)) & 0x3f);
	}
	return (char*)cur;
}

/*
 * render a draw 's' into the parent 'd' at x,y
 * this is the 'blitter' really
 */
static void
_sc_draw_render(
	sc_draw_t *dst,
	sc_draw_t *src,
	int dst_x, int dst_y )
{
	for (int y = 0; y < src->h; y++) {
		int dy = dst_y + y;
		if (dy < 0 || dy >= dst->h)
			continue;
		if (y < src->line.count) {	// non-empty line
			sc_line_t * l = &src->line.e[y];
			int dx = dst_x, sx = 0;
			#if 0
			int leading = 0;// count leading spaces, for center/right justify
			for (int i = 0; i < l->count; i++)
				if (l->e[i].g == 0 || l->e[i].g == ' ')
					leading++;
			#endif
			switch (src->justify) {
				case SC_WIN_JUSTIFY_CENTER:
					if (l->count > src->w)
						sx = (l->count - src->w) / 2;
					else
						dx += (src->w - l->count) / 2;
					break;
				case SC_WIN_JUSTIFY_RIGHT:
					if (l->count > src->w)
						sx = l->count - src->w;
					else
						dx += src->w - l->count;
					break;
				case SC_WIN_JUSTIFY_LEFT:
				default:
					break;
			}
			for (; dx < (dst_x + src->w) && sx < l->count; sx++, dx++) {
				if ((dx >= 0 && dx < dst->w)) {
					sc_draw_store_xy(dst, &l->e[sx], dx, dy);
				}
			}
			for (; dx < (dst_x + src->w); dx++) {
				if ((dx >= 0 && dx < dst->w)) {
					sc_draw_store_xy(dst, NULL, dx, dy);
				}
			}
		} else {	// empty line, clear it.
			for (int x = 0; x < src->w; x++) {
				int dx = dst_x + x;
				if ((dx >= 0 && dx < dst->w)) {
					sc_draw_store_xy(dst, NULL, dx, dy);
				}
			}
		}
	}
}

static void
_sc_render_subs(
	sc_win_t *s,
	int force)
{
	sc_win_t *sb;
	if (!s->draw.dirty) {
		TAILQ_FOREACH_REVERSE(sb, &s->sub, sub, self) {
			if (sb->draw.dirty) {
				s->draw.dirty = 1;
				break;
			}
		}
	}
	if (!s->draw.dirty)
		return;
	s->draw.dirty = 0;
	TAILQ_FOREACH_REVERSE(sb, &s->sub, sub, self) {
		_sc_render_subs(sb, 0);
	}
	if (!s->parent)
		return;
	if (s->driver && s->driver->draw)
		s->driver->draw(s);

	_sc_draw_render(&s->parent->draw, &s->draw, s->x, s->y);
}

static void
_sc_render_flush_style(
	sc_t * sc,
	sc_buf_t * out)
{
	if (sc->render.style_count == 0)
		return;
	sc_buf_t seq = {};
	sc_style_t n = sc->render.style;
	sc_style_t o = sc->render.old_style;
#if 0
	printf("%s offset %d count %d old %x new %x\n", __func__,
			sc->render.style_insert,
			sc->render.style_count,
			o.raw, n.raw);
#endif
	sc->render.old_style = sc->render.style;
	sc->render.style_count = 0;

	int c = 0;
	sc_buf_concat(&seq, (uint8_t*)"\033[", 0);
	if (n.bold != o.bold)
		sc_buf_printf(&seq, "%s%d", c++ > 0 ? ";" : "",  n.bold ? 1 : 22);
	if (n.under != o.under)
		sc_buf_printf(&seq, "%s%d", c++ > 0 ? ";" : "", n.under ? 4 : 24);
	if (n.invert != o.invert)
		sc_buf_printf(&seq, "%s%d", c++ > 0 ? ";" : "", n.invert ? 7 : 27);
	if (n.fore != o.fore) {
		if (!n.has_fore)
			sc_buf_printf(&seq, "%s0", c++ > 0 ? ";" : "");
		else if (n.fore < 8)
			sc_buf_printf(&seq, "%s%d", c++ > 0 ? ";" : "", 30 + n.fore);
		else
			sc_buf_printf(&seq, "%s38;5;%d", c++ > 0 ? ";" : "", n.fore);
	}
	if (n.back != o.back) {
		if (!n.has_back)
			sc_buf_printf(&seq, "%s0", c++ > 0 ? ";" : "");
		else if (n.back < 8)
			sc_buf_printf(&seq, "%s%d", c++ > 0 ? ";" : "", 40 + n.back);
		else
			sc_buf_printf(&seq, "%s48;5;%d", c++ > 0 ? ";" : "", n.back);
	}
	if (!c)
		sc_buf_add(&seq, '0');
	sc_buf_add(&seq, 'm');
#if 0
	printf("     Insert offset %d, %d bytes seq: ",
		   sc->render.style_insert, seq.count);
	for (int i = 0; i < seq.count; i++) {
		if (seq.e[i] < ' ')
			printf("%02x", seq.e[i]);
		else printf("%c", seq.e[i]);
	}
	printf("\n");
#endif
	sc_buf_insert(out, sc->render.style_insert, seq.e, seq.count);
	sc_buf_free(&seq);
}

static void
_sc_render_flush_space(
	sc_t * sc,
	sc_buf_t * out)
{
	if (sc->render.space_count > 4) {
		//	printf("HT %d\n", sc->render.space_count);
		sc_buf_printf(out, "\033[%dC", sc->render.space_count);
	} else
		for (int si = 0; si < sc->render.space_count; si++)
			sc_buf_add(out, ' ');
	sc->render.space_count = 0;
}

static void
_sc_render_glyph(
	sc_t *sc,
	sc_buf_t * o,
	sc_line_t *l,
	unsigned int x)
{
	sc_glyph_t *g = &l->e[x];

	/*
	 * Count the characters we output with the same style. If the style change,
	 * go back and insert the style setting sequence at the last point we
	 * detected a change.
	 */
	if (g->style.raw != sc->render.style.raw) {
		_sc_render_flush_space(sc, o);
		_sc_render_flush_style(sc, o);
		// next insert point for a style change
		sc->render.style_insert = o->count;
		sc->render.style_count = 1;
		sc->render.style.raw = g->style.raw;
	} else
		sc->render.style_count++;
	// if we had a space run, and find something else, flush spaces.
	switch (g->g) {
		/*
		 * We count space, don't store them; if a run is bigger than 5,
		 * it's worth using the "move cursor" sequence. That only works if
		 * there are no style change of course
		 */
		case 0: // default/space
		case ' ':
			sc->render.space_count++;
			break;
		default: {
			if (sc->render.space_count)
				_sc_render_flush_space(sc, o);
			if (g->g > 0x7f) {
				char ut[8];
				int len = _sc_render_utf8_glyph(ut, g->g) - ut;
				sc_buf_concat(o, (uint8_t*)ut, len);
			} else
				sc_buf_add(o, g->g);
		}	break;
	}
}

static void
_sc_render_line(
	sc_t *sc,
	sc_win_t *s,
	sc_buf_t * o,
	sc_line_t *l )
{
	sc_buf_concat(o, (uint8_t*)"\033[0m", 0);	// CLREOL
	if (l->count) {
		sc_buf_concat(o, (uint8_t*)"\033[K", 0);	// CLREOL
	}
	sc->render.pt = NULL;
	sc->render.style.raw = 0;
	sc->render.style_count = 0;
	sc->render.space_count = 0;
	for (int x = 0; x < l->count; x++) {
		_sc_render_glyph(sc, o, l, x);
	}
	if (sc->render.style.raw != sc->render.old_style.raw)
		_sc_render_flush_style(sc, o);
}

int
sc_render(
	sc_t *sc,
	uint8_t flags)
{
	SC_GET(sc);
	sc_win_t *s = &sc->screen;

	_sc_render_subs(s, 0);

	sc_buf_free(&sc->output);
	if (sc->output.lines) {
		sc_buf_printf(&sc->output, "\033[%dA", sc->output.lines);
	}
	sc_buf_push(&sc->output, '\r');
#ifdef DEBUG
	if (sc->output.lines) {	// debug
		sc_buf_printf(&sc->output, "%d lines. Term %dx%d\r",
					  sc->output.lines, sc->screen.w, sc->screen.h);
	}
#endif
	sc_draw_t *d = &s->draw;
	sc->output.lines = 0;
	for (int y = 0; y < d->line.count; y++) {
		sc_line_t * l = &d->line.e[y];
		_sc_render_line(sc, s, &sc->output, l);
		sc_buf_push(&sc->output, '\n');
		sc->output.lines++;
	}
	//printf("\033[0mOut is %d bytes\n", sc->output.count);
	if (!(flags & SC_RENDER_NO_STDOUT))
		fwrite(sc->output.e, sc->output.count, 1, stdout);
	return 0;
}
