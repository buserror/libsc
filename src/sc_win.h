/*
 * sc_win.h
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Create a new window object for 'sc' in parent window object 'parent'.
 * It is not sized in terms of character size -- instance_size is the size
 * in byte of the returned object, used by sc_box for the moment. */
sc_win_t *
sc_win_new(
	sc_t * sc,
	sc_win_t * parent /* = NULL */,
	size_t instance_size /* = 0 */);
/* Clear any internal character buffer for window 's' */
void
sc_win_clear(
	sc_win_t *s );
/* Mark s and (all) its parents as dirty (needing redraw) */
void
sc_win_dirty(
	sc_win_t *s );
/* dispose storage, and any subwindows */
void
sc_win_dispose(
	sc_win_t *s );
/* Get current window for 'sc' */
sc_win_t *
sc_win_get(
	sc_t * sc);
/* set current window (the one we draw in) */
sc_win_t *
sc_win_set(
	sc_t * sc,
	sc_win_t * s);
/* set drawing position x,y in window s */
void
sc_win_goto(
	sc_win_t *s,
	int x, int y);
/* Call sc_add with the printf-equivalent string */
int
sc_win_printf(
	sc_win_t *s,
	const char *f,
	...);

/* static */ sc_win_t *
_sc_win_init(
	sc_t * sc,
	sc_win_t *parent,
	sc_win_t *s);

//#ifdef DEBUG
void
sc_win_dump(sc_win_t *w);
//#endif
