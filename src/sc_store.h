/*
 * sc_store.h
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Add a string what to the current window/pos in sc. 'what' will be parsed for
 * (some) escape sequences, like attribute and (some) cursor positioning */
int
sc_add(
	sc_t *sc,
	const char *what,
	unsigned int l);
/* Call sc_add with the printf-equivalent string */
int
sc_printf(
		sc_t *sc,
		const char *f,
		...);

/* Store a glyph g to x,y in window 's'.
 * If 'g' is NULL, return the address of the glyph in that window,
 * allowing changing the glyph on the fly */
sc_glyph_t *
sc_draw_store_xy(
	sc_draw_t *s,
	const sc_glyph_t *g,
	int x, int y);
/* static */ int
_sc_draw_add_store(
	sc_draw_t *s,
	uint32_t c,
	uint8_t flags);



