/*
 * sc.h
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SC_H__
#define __SC_H__

#include <stdint.h>
#include <string.h>
#include <sys/queue.h>
#include "c_array.h"

enum {
	SC_LINE_MAX = 100,
};

typedef union sc_style_t {
	struct {
		uint32_t 	fore : 8, back : 8,
					has_fore : 1, has_back : 1,
					bold : 1, under : 1, invert : 1;
	};
	uint32_t 	raw;
} sc_style_t;

// a glyph. it's big. handles unicode
typedef union {
	struct {
		uint32_t 	g;
		sc_style_t	style;
	};
	uint64_t raw[1];
} __attribute__((packed)) sc_glyph_t;

DECLARE_C_ARRAY(sc_glyph_t, sc_line, 8,
				uint32_t align_h: 4, align_v : 4; );
DECLARE_C_ARRAY(sc_line_t, sc_lines, 2);

struct sc_t;
struct sc_win_t;

enum {
	SC_WIN_JUSTIFY_LEFT = 0,
	SC_WIN_JUSTIFY_CENTER,
	SC_WIN_JUSTIFY_RIGHT,
};

typedef struct sc_win_driver_t {
	int kind;
	void (*draw)(struct sc_win_t *);
	void (*free)(struct sc_win_t *);
} sc_win_driver_t;

typedef struct sc_draw_t {
	unsigned int 				dirty : 1,
								justify: 2,	// SC_WIN_JUSTIFY_*
								kind : 8,	// for custom windows/boxes
								draw_style : 4;	// for draw_cb
	uint8_t 					c_x, c_y;	// current cursor position
	uint8_t 					w, h;		// size
	sc_style_t 					style; 		// current style
	sc_lines_t 					line;
} sc_draw_t;

DECLARE_C_ARRAY(sc_draw_t, sc_draw_array, 2);

typedef struct sc_win_t {
	sc_draw_t					draw;
	struct sc_t * 				sc;
	struct sc_win_t * 			parent;
	TAILQ_HEAD(sub,sc_win_t)	sub;
	sc_win_driver_t const *		driver;
	TAILQ_ENTRY(sc_win_t)		self;
	uint8_t 					x, y;		// position in parent window
} sc_win_t;

#include "sc_buf.h"

enum {
	SC_PARTIAL		= (1 << 0),		// don't erase screen
	SC_REDRAW		= (1 << 2),		// supports redrawing same lines
};

typedef struct sc_add_context_t {
	void *						pt;
	unsigned int 				pcount;	// parameter count (+1)
	unsigned int 				p[16];
	unsigned int 				utf8_state;
	uint32_t 					utf8_glyph;
} sc_add_context_t;

typedef struct sc_render_context_t {
	void *						pt;
	unsigned int 				space_count;	// space count
	sc_style_t					style, old_style; // detect style change boundary
	unsigned int				style_count;
	unsigned int				style_insert;	// insert offset for the style sequence
} sc_render_context_t;

typedef struct sc_t {
	uint32_t					flags;
	sc_win_t 					screen; // main rendering window
	sc_win_t *					current;
	sc_buf_t					output;	// last generated output
	sc_add_context_t			add;	// add context
	sc_render_context_t			render;	// render context
} sc_t;

/* Create a new sc instance. This is recommended before you do anything,
 * but it is currently mostly optional as one will get created anyway */
sc_t *
sc_new(
		uint32_t flags);

void
sc_dispose(
		sc_t * sc);
/* Move cursor to x, y in 'current window' in sc */
void
sc_goto(
		sc_t *sc,
		int x, int y);
/* Non-blocking get character from console, return 0, or 1+characters sequence */
unsigned int
sc_getch(
		sc_t * sc,
		unsigned int timeout_ms);

#include "sc_draw.h"
#include "sc_store.h"
#include "sc_render.h"
#include "sc_win.h"
#include "sc_box.h"

#endif /* __SC_H__ */

#if defined(SC_H_INTERNAL) || defined(SC_H_IMPLEMENTATION)

#include <stdio.h>

extern sc_t * g_sc;	// in sc_base.c

#define SC_GET(_sc) ((_sc) ? (_sc) : \
					g_sc ? ((_sc) = g_sc) : \
					((_sc) = (g_sc) = sc_new(0)))

IMPLEMENT_C_ARRAY(sc_line);
IMPLEMENT_C_ARRAY(sc_lines);
IMPLEMENT_C_ARRAY(sc_buf);
IMPLEMENT_C_ARRAY(sc_draw_array);

#endif

#ifdef SC_H_IMPLEMENTATION
// mini protothreads
#include "minipt.h"
#include "sc_buf.c"
#include "sc_store.c"
#include "sc_draw.c"
#include "sc_box.c"
#include "sc_render.c"
#include "sc_win.c"
#include "sc_base.c"

#endif /* SC_H_IMPLEMENTATION */
