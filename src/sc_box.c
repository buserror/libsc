/*
 * sc_box.c
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SC_H_INTERNAL
#include "sc_base.h"

typedef struct sc_box_t {
	sc_win_t w;
	char * title;
} sc_box_t;

typedef union sc_box_style_t {
	struct {
		uint16_t tl,tr,bl,br,h,v,cross;
	};
	uint16_t g[7];
} sc_box_style_t;

static const sc_box_style_t sc_box_style[] = {
	[SC_BOX_PLAIN] = {
		.g = {0x250c,0x2510,0x2514,0x2518,0x2500,0x2502,0x253c}},
	[SC_BOX_ROUND] = {
		.g = {0x256d,0x256e,0x2570,0x256f,0x2500,0x2502,0x253c}},
};

static int
_sc_box_render(
	sc_win_t *s,
	const sc_box_style_t * style,
	const int x, int y, int w, int h )
{
	if (w < 3 || h < 3) return -1;

	sc_win_goto(s, x, y);
	_sc_add_store(s, style->tl, 0);
	for (int sx = x+1; sx < (x + w - 1); sx++)
		_sc_add_store(s, style->h, 0);
	_sc_add_store(s, style->tr, 0);
	sc_win_goto(s, x, y + h-1);
	_sc_add_store(s, style->bl, 0);
	for (int sx = x+1; sx < (x + w - 1); sx++)
		_sc_add_store(s, style->h, 0);
	_sc_add_store(s, style->br, 0);

	for (int sy = y + 1; sy < (y + h - 1); sy++) {
		sc_win_goto(s, x, sy);
		_sc_add_store(s, style->v, 0);
		sc_win_goto(s, x + w - 1, sy);
		_sc_add_store(s, style->v, 0);
	}
	return 0;
}

static void
_sc_box_render_cb(
	sc_win_t *s)
{
	sc_box_t * box = (sc_box_t*)s;
	_sc_box_render(s, &sc_box_style[s->draw_style], 0, 0, s->w, s->h);
	if (box->title) {
		sc_win_t *save = s->sc->current;
		sc_win_set(s->sc, s);
		sc_win_goto(s, 2, 0);
		sc_printf(s->sc, "%s",  box->title);
		sc_win_set(s->sc, save);
	}
}

static void
_sc_box_free_cb(
	sc_win_t *s)
{
	sc_box_t * box = (sc_box_t*)s;
	if (box->title)
		free(box->title);
}

static const sc_win_driver_t _driver = {
	.kind = 100, // TODO: make a constant somewhere
	.draw = _sc_box_render_cb,
	.free = _sc_box_free_cb,
};

sc_win_t *
sc_box(
	sc_win_t *parent,
	int x, int y,
	int w, int h,
	uint8_t flags )
{
	sc_t * sc = parent ? parent->sc : NULL;
	SC_GET(sc);
	if (!parent)
		parent = sc->current;
	if (!parent)
		parent = sc->current = &sc->screen;

	sc_win_t *s = sc_win_new(sc, parent, sizeof(sc_box_t));
	s->x = x; s->y = y; s->w = w; s->h = h;
	s->driver = &_driver;
	s->draw_style = flags;
	s->kind = SC_BOX_PLAIN;

	sc_win_t *sub = sc_win_new(sc, s, 0);
	sub->x = 1; sub->y = 1; sub->w = w - 2; sub->h = h - 2;
	sc->current = sub;

	return sub;
}

int
sc_box_title_set(
	sc_win_t *box_in,
	const char *title)
{
	if (!box_in || !box_in->parent || box_in->parent->kind != SC_BOX_PLAIN)
		return -1;
	sc_box_t * box = (sc_box_t*)box_in->parent;

	if (box->title)
		free(box->title);
	if (title)
		box->title = strdup(title);
	return 0;
}

