/*
 * sc_render.h
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum {
	// create the output, but don't send it to stdout
	SC_RENDER_NO_STDOUT = (1 << 0),
};

int
sc_render(
	sc_t *sc,
	uint8_t flags);
