/*
 * sc_draw.h
 *
 * Copyright (C) 2023 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


void
sc_draw_clear(
		struct sc_draw_t * s);

void
sc_draw_dirty(
		struct sc_draw_t * s);

void
sc_draw_goto(
		struct sc_draw_t *s,
		int x, int y);

void
sc_draw_dispose(
		struct sc_draw_t *s);
