/*
 * sc_draw.h
 *
 * Copyright (C) 2023 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//! Clear context and contents and reset cursor to 0,0
void
sc_draw_clear(
		struct sc_draw_t * s);
//! Mark the context as dirty, ie, needs to be redrawn
void
sc_draw_dirty(
		struct sc_draw_t * s);
//! Set the cursor position in the context (does not allocate lines)
void
sc_draw_goto(
		struct sc_draw_t *s,
		int x, int y);
//! Disposes of all the lines in the draw context
void
sc_draw_dispose(
		struct sc_draw_t *s);
//! like sc_add but using a add context and a sc_draw ('add' is optional)
int
sc_draw_add(
	struct sc_add_context_t *add,
	struct sc_draw_t *s,
	const char *what,
	unsigned int l);
//! Same as printf with a va_list argument ('add' is optional)
int
sc_draw_vprintf(
	struct sc_add_context_t *add,
	struct sc_draw_t *s,
	const char *f,
	va_list ap);
//! printf in the draw context at the current x,y position ('add' is optional)
int
sc_draw_printf(
	struct sc_add_context_t *add,
	struct sc_draw_t *s,
	const char *f,
	...);
