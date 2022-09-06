/*
 * sc_box.h
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum {
	SC_BOX_PLAIN = 0,
	SC_BOX_ROUND,
};

/*
 * Create a box, and return the *contents* store, sized w-2 x h-2
 */
sc_win_t *
sc_box(
	sc_win_t *parent,
	int x, int y,
	int w, int h,
	uint8_t flags );
/* Set title for the box *containing* box, ie the return value of sc_box() */
int
sc_box_title_set(
	sc_win_t *box,
	const char *title /* or NULL */);
