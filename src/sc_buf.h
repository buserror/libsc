/*
 * sc_buf.h
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include "c_array.h"

DECLARE_C_ARRAY(uint8_t, sc_buf, 16,
				unsigned int lines; );

/* Concat buffer 'e' sized 'c' to buffer 'b' */
int
sc_buf_concat(
	sc_buf_t *b,
	uint8_t *e,
	unsigned int c);

/* Concat printf equivalent text to buffer b */
int
sc_buf_vprintf(
	sc_buf_t *b,
	const char *f,
	va_list ap);
/* Concat printf equivalent text to buffer b */
int
sc_buf_printf(
	sc_buf_t *b,
	const char *f,
	...);
