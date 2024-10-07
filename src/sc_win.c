/*
 * sc_win.c
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SC_H_INTERNAL
#include "sc_base.h"

sc_win_t *
_sc_win_init(
	sc_t * sc,
	sc_win_t *parent,
	sc_win_t *s)
{
	s->sc = sc;
	s->parent = parent;
	TAILQ_INIT(&s->sub);
	if (parent)
		TAILQ_INSERT_HEAD(&parent->sub, s, self);
	return s;
}

sc_win_t *
sc_win_new(
	sc_t * sc,
	sc_win_t * parent,
	size_t instance_size)
{
	SC_GET(sc);
	if (!instance_size)
		instance_size = sizeof(sc_win_t);
	sc_win_t * s = calloc(1, instance_size);
	if (!parent)
		parent = &sc->screen;
	return _sc_win_init(sc, parent, s);
}

void
sc_win_clear(
	sc_win_t *s )
{
	sc_draw_clear(&s->draw);
}

void
sc_win_dirty(
	sc_win_t *s )
{
	sc_win_t *p = s;
	do {
		sc_draw_dirty(&p->draw);
		p = p->parent;
	} while(p);
}

void
sc_win_dispose(
	sc_win_t *s )
{
	if (s->parent)
		TAILQ_REMOVE(&s->parent->sub, s, self);
	// delete any subwindows
	sc_win_t *sb;
	while ((sb = TAILQ_FIRST(&s->sub)) != NULL) {
		sc_win_dispose(sb);
	}
	sc_draw_dispose(&s->draw);
	if (s->driver && s->driver->free)
		s->driver->free(s);
	// don't delete detached/main screen
	if (s->parent)
		free(s);
}

sc_win_t *
sc_win_get(
	sc_t * sc)
{
	SC_GET(sc);
	return sc->current ? sc->current : &sc->screen;
}

sc_win_t *
sc_win_set(
	sc_t * sc,
	sc_win_t * s)
{
	SC_GET(sc);
	if (!s)
		s = &sc->screen;
	sc->current = s;
	return sc->current;
}

void
sc_win_goto(
	sc_win_t *s,
	int x, int y)
{
	sc_draw_goto(&s->draw, x, y);
}

int
sc_win_printf(
	sc_win_t *s,
	const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int r = sc_draw_vprintf(&s->sc->add, &s->draw, fmt, ap);
	va_end(ap);
	if (s->draw.dirty)
		sc_win_dirty(s);
	return r;
}

//#ifdef DEBUG
void
sc_draw_dump(
		sc_draw_t *s)
{
	printf("%s %d lines. dirty:%d\n", __func__, s->line.count, s->dirty);
	for (int y = 0; y < s->line.count; y++) {
		sc_line_t * l = &s->line.e[y];
		for (int x = 0; x < l->count; x++)
			printf("%08x ", l->e[x].style.raw);
		printf("\n");
		for (int x = 0; x < l->count; x++)
			printf("%06x:%c ", l->e[x].g, l->e[x].g);
		printf("\n");
	}
}
//#endif
