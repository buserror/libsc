// #include "sc_base.h"
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
// #include "c_array.h"
#ifndef __C_ARRAY_H___
#define __C_ARRAY_H___

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef C_ARRAY_INLINE
#define C_ARRAY_INLINE inline
#endif
#ifndef C_ARRAY_SIZE_TYPE
#define C_ARRAY_SIZE_TYPE uint32_t
#endif
/* New compilers don't like unused static functions. However,
 * we do like 'static inlines' for these small accessors,
 * so we mark them as 'unused'. It stops it complaining */
#ifdef __GNUC__
#define C_ARRAY_DECL static  __attribute__ ((unused))
#else
#define C_ARRAY_DECL static
#endif


#define DECLARE_C_ARRAY(__type, __name, __page, __args...) \
enum { __name##_page_size = __page }; \
typedef __type __name##_element_t; \
typedef C_ARRAY_SIZE_TYPE __name##_count_t; \
typedef struct __name##_t {\
	volatile __name##_count_t count;\
	volatile __name##_count_t size;\
	__name##_element_t * e;\
	__args \
} __name##_t, *__name##_p;

#define C_ARRAY_NULL { 0, 0, NULL }

#ifndef NO_DECL
#define IMPLEMENT_C_ARRAY(__name) \
C_ARRAY_DECL C_ARRAY_INLINE \
	void __name##_init(\
			__name##_p a) \
{\
	static const __name##_t zero = {}; \
	if (!a) return;\
	*a = zero;\
}\
C_ARRAY_DECL C_ARRAY_INLINE \
	void __name##_free(\
			__name##_p a) \
{\
	if (!a) return;\
	if (a->e) free(a->e);\
	a->count = a->size = 0;\
	a->e = NULL;\
}\
C_ARRAY_DECL C_ARRAY_INLINE \
	void __name##_clear(\
			__name##_p a) \
{\
	if (!a) return;\
	a->count = 0;\
}\
C_ARRAY_DECL C_ARRAY_INLINE \
	void __name##_realloc(\
			__name##_p a, __name##_count_t size) \
{\
	if (!a || a->size == size) return; \
	if (size == 0) { if (a->e) free(a->e); a->e = NULL; } \
	else a->e = (__name##_element_t*)realloc(a->e, \
						size * sizeof(__name##_element_t));\
	a->size = size; \
}\
C_ARRAY_DECL C_ARRAY_INLINE \
	void __name##_trim(\
			__name##_p a) \
{\
	if (!a) return;\
	__name##_count_t n = a->count + __name##_page_size;\
	n -= (n % __name##_page_size);\
	if (n != a->size)\
		__name##_realloc(a, n);\
}\
C_ARRAY_DECL C_ARRAY_INLINE \
	__name##_element_t * __name##_get_ptr(\
			__name##_p a, __name##_count_t index) \
{\
	if (!a) return NULL;\
	if (index > a->count) index = a->count;\
	return index < a->count ? a->e + index : NULL;\
}\
C_ARRAY_DECL C_ARRAY_INLINE \
	__name##_count_t __name##_add(\
			__name##_p a, __name##_element_t e) \
{\
	if (!a) return 0;\
	if (a->count + 1 >= a->size)\
		__name##_realloc(a, a->size + __name##_page_size);\
	a->e[a->count++] = e;\
	return a->count;\
}\
C_ARRAY_DECL C_ARRAY_INLINE \
	__name##_count_t __name##_push(\
			__name##_p a, __name##_element_t e) \
{\
	return __name##_add(a, e);\
}\
C_ARRAY_DECL C_ARRAY_INLINE \
	int __name##_pop(\
			__name##_p a, __name##_element_t *e) \
{\
	if (a->count) { *e = a->e[--a->count]; return 1; } \
	return 0;\
}\
C_ARRAY_DECL C_ARRAY_INLINE \
	__name##_count_t __name##_insert(\
			__name##_p a, __name##_count_t index, \
			const __name##_element_t * e, __name##_count_t count) \
{\
	if (!a || !e || !count) return 0;\
	if (index > a->count) index = a->count;\
	if (a->count + count >= a->size) \
		__name##_realloc(a, (((a->count + count) / __name##_page_size)+1) * __name##_page_size);\
	if (index < a->count)\
		memmove(&a->e[index + count], &a->e[index], \
				(a->count - index) * sizeof(__name##_element_t));\
	memmove(&a->e[index], e, count * sizeof(__name##_element_t));\
	a->count += count;\
	return a->count;\
}\
C_ARRAY_DECL C_ARRAY_INLINE \
	__name##_count_t __name##_append(\
			__name##_p a, \
			const __name##_element_t * e, __name##_count_t count) \
{\
	if (!a) return 0;\
	return __name##_insert(a, a->count, e, count);\
}\
C_ARRAY_DECL C_ARRAY_INLINE \
	__name##_count_t __name##_delete(\
			__name##_p a, __name##_count_t index, __name##_count_t count) \
{\
	if (!a) return 0;\
	if (index > a->count) index = a->count;\
	if (index + count > a->count) \
		count = a->count - index;\
	if (count && a->count - index) { \
		memmove(&a->e[index], &a->e[index + count], \
				(a->count - index - count) * sizeof(__name##_element_t));\
	}\
	a->count -= count;\
	return a->count;\
}
#else /* NO_DECL */

#define IMPLEMENT_C_ARRAY(__name)

#endif

#endif /* __C_ARRAY_H___ */

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

// #include "sc_buf.h"
#include <stdarg.h>

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

// #include "sc_draw.h"
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
// #include "sc_store.h"
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
_sc_draw_store_add(
	sc_draw_t *s,
	uint32_t c,
	uint8_t flags);



// #include "sc_render.h"
enum {
	// create the output, but don't send it to stdout
	SC_RENDER_NO_STDOUT = (1 << 0),
};

int
sc_render(
	sc_t *sc,
	uint8_t flags);
// #include "sc_win.h"
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
// #include "sc_box.h"
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
// #include "minipt.h"
#ifndef MPTOOLS_INCLUDE_MINIPT_H_
#define MPTOOLS_INCLUDE_MINIPT_H_

/*
 * Mini Protothread.
 *
 * A thread or a coroutine would use a stack; this won't,
 * Use an old gcc trick of being able to goto and indirect label.
 * There are a few caveats: no persistent local variables, as you can't
 * have a consistent stack frame. It's easy to work around tho.
 */
#define _CONCAT2(s1, s2) s1##s2
#define _CONCAT(s1, s2) _CONCAT2(s1, s2)

/* this wierd thing with the union is for gcc 12, which doesn't like us
 * storing the address of a 'local variable' (which is the label!) */
static inline void _set_gcc_ptr_workaround(void **d, void *s) {
#pragma GCC diagnostic push
#if __GNUC__ >= 12
#pragma GCC diagnostic ignored "-Wdangling-pointer"
#endif
	*d = s;
#pragma GCC diagnostic pop
}
#define pt_start(_pt) do { \
		if (_pt) goto *_pt; \
	} while (0);
#define pt_end(_pt) do { \
		(_pt) = NULL; \
		_pt_exit: ; \
	} while(0);
#define pt_finish(_pt) do { \
		(_pt) = NULL; \
		goto _pt_exit;\
	} while(0);
#define pt_yield(_pt) do { \
		_set_gcc_ptr_workaround(&(_pt), &&_CONCAT(_label, __LINE__));\
		goto _pt_exit;\
		_CONCAT(_label, __LINE__): ; \
	} while (0);

#define pt_wait(_pt, _condition) do { \
		while (!(_condition)) \
			pt_yield(_pt); \
	} while (0);


#ifdef NEVER
/*
 * This version is superior as it allows calling functions and keeping
 * a context, but I never actually had a /need/ for this, yet
 */

struct pt_t {
	unsigned int sp;
	void * st[32];
	void * ctx[32];
} pt_t;

#define pt_start(_pt) do { \
		if ((_pt)->st[(_pt)->sp]) goto *((_pt)->st[(_pt)->sp]); \
	} while (0);
#define pt_end(_pt) do { (_pt)->st[(_pt)->sp] = NULL; return; } while(0);
#define pt_yield(_pt) do { \
		_set_gcc_ptr_workaround(&(_pt)->st[(_pt)->sp], &&_CONCAT(_label, __LINE__));\
		return;\
		_CONCAT(_label, __LINE__): ; \
	} while (0);
#define pt_wait(_pt, _condition) do { \
		while (!(_condition)) \
			pt_yield(_pt); \
	} while (0);
#define pt_call(_pt, _func) do { \
		(_pt)->sp++; \
		(_pt)->st[(_pt)->sp] = NULL; \
		do { \
			_func(_pt); \
		} while ((_pt)->st[(_pt)->sp]); \
		(_pt)->sp--; \
	} while (0);
#define pt_ctx(_pt) ((_pt)->ctx[(_pt)->sp])

void my_minit_func(struct pt_t * p) {
	pt_start(p);
	pt_ctx(p) = calloc(1, sizeof(int));
	printf("%s start %p\n", __func__, pt_ctx(p));
	pt_yield(p);
	int * ctx = pt_ctx(p);
	printf("%s loop %p\n", __func__, pt_ctx(p));
	for (; *ctx < 10; ctx[0]++) {
		printf("   loop %d\n", *ctx);
		pt_yield(p);
		ctx = pt_ctx(p);
	}
	printf("%s done %p\n", __func__, pt_ctx(p));
	free(pt_ctx(p));
	pt_ctx(p) = NULL;
	pt_end(p);
}

void my_minit(struct pt_t * p) {
	pt_start(p);
	printf("%s start\n", __func__);
	pt_call(p, my_minit_func);
	printf("%s done\n", __func__);
	pt_end(p);
}

int main() {
	struct pt_t pt = {};

	pt_call(&pt, my_minit);
}
/*
tcc -run pt_call_test.c
my_minit start
my_minit_func start 0x555a68d970b0
my_minit_func loop 0x555a68d970b0
   loop 0
   loop 1
   loop 2
   loop 3
   loop 4
   loop 5
   loop 6
   loop 7
   loop 8
   loop 9
my_minit_func done 0x555a68d970b0
my_minit done
*/

#endif // NEVER

#endif /* MPTOOLS_INCLUDE_MINIPT_H_ */
// #include "sc_buf.c"

int
sc_buf_concat(
	sc_buf_t *b,
	uint8_t *e,
	unsigned int c)
{
	if (!b || !e) return -1;
	if (!c) c = strlen((char*)e);
	return sc_buf_insert(b, b->count, e, c);
}

int
sc_buf_vprintf(
	sc_buf_t *b,
	const char *f,
	va_list ap)
{
	if (!b || !f) return -1;
	va_list c;
	va_copy(c, ap);
	int l = vsnprintf(NULL, 0, f, c);
	if ((b->count + l) >= b->size)
		sc_buf_realloc(b, b->count + l + 1);
	va_end(c);
	/*int dl = */vsnprintf((char*)b->e + b->count, l + 1, f, ap);
	b->count += l;
	return l;
}

int
sc_buf_printf(
	sc_buf_t *b,
	const char *f,
	...)
{
	if (!b || !f) return -1;
	va_list ap;
	va_start(ap, f);
	int res = sc_buf_vprintf(b, f, ap);
	va_end(ap);
	return res;
}

// #include "sc_store.c"

enum {
	SC_ADD_NO_ADVANCE = (1 << 0),
};

sc_glyph_t *
sc_draw_store_xy(
	sc_draw_t *s,
	const sc_glyph_t *g,
	int x, int y)
{
	if (x < 0 || y < 0)
		return NULL;
	while (s->line.count <= y) {
		static const sc_line_t zero = {};
		sc_lines_add(&s->line, zero);
	}
	sc_line_t * l = &s->line.e[y];
	const sc_glyph_t zero = {};
	while (l->count <= x) {
		sc_line_add(l, zero);
	}
	if (g) {
		if (l->e[x].g != g->g || l->e[x].style.raw != g->style.raw) {
			sc_draw_dirty(s);
			l->e[x] = *g;
		}
	} else {
		if (l->e[x].g || l->e[x].style.raw) {
			sc_draw_dirty(s);
			l->e[x] = zero;
		}
	}
	return &l->e[x];
}

int
_sc_draw_store_add(
	sc_draw_t *s,
	uint32_t c,
	uint8_t flags)
{
	while (s->line.count <= s->c_y) {
		static const sc_line_t zero = {};
		sc_lines_add(&s->line, zero);
	}
	sc_line_t * l = &s->line.e[s->c_y];
	while (l->count <= s->c_x) {
		const sc_glyph_t zero = {};
		sc_line_add(l, zero);
	}
	sc_glyph_t g = { .g = c, .style = s->style };
	if (l->e[s->c_x].raw != g.raw) {
		l->e[s->c_x] = g;
		sc_draw_dirty(s);
	}
	if (!(flags & SC_ADD_NO_ADVANCE)) {
		s->c_x++;
		if (s->c_x >= SC_LINE_MAX) {
			s->c_x = 0;
			s->c_y++;
		}
	}
	return s->dirty;
}

static void
_sc_draw_store_csi(
	sc_add_context_t *add,
	sc_draw_t *s,
	uint8_t c)
{
	switch (c) {
		case 'A': { // move Y up
			int cl = 1;
			if (add->pcount)
				cl = add->p[0];
			if (s->c_y > cl)
				s->c_y = 0;
			else
				s->c_y -= cl;
		}	break;
		case 'G': { // Move absolute X
			int cl = 1;
			if (add->pcount)
				cl = add->p[0];
			if (cl) cl--;	// 1 based to zero based
			s->c_x = cl;
		}	break;
		case 'm': {	// style/colors
			for (int pi = 0; pi < add->pcount; pi++) {
				//	printf("CSI %d m\n", add->p[pi]);
				switch (add->p[pi]) {
					case 0:
						s->style.raw = 0;
						break;
					case 1:
					case 21:
						s->style.bold = add->p[pi] < 10;
						break;
					case 4:
					case 24:
						s->style.under = add->p[pi] < 10;
						break;
					case 7:
					case 27:
						s->style.invert = add->p[pi] < 10;
						break;
					case 22:
						s->style.bold = 0;
						break;
					case 30 ... 37:
						s->style.fore = add->p[pi] - 30;
						s->style.has_fore = 1;
						break;
					case 38: {	// 256 colors
						if (add->pcount - pi < 3 || add->p[pi + 1] != 5)
							break;
						s->style.fore = add->p[pi + 2];
						s->style.has_fore = 1;
						pi += 3;
					}	break;
					case 39:
						s->style.fore = 0;
						s->style.has_fore = 0;
						break;
					case 40 ... 47:
						s->style.back = add->p[pi] - 40;
						s->style.has_back = 1;
						break;
					case 48: {	// 256 colors
						if (add->pcount - pi < 3 || add->p[pi + 1] != 5)
							break;
						s->style.back = add->p[pi + 2];
						s->style.has_back = 1;
						pi += 3;
					}	break;
					case 49:
						s->style.back = 0;
						s->style.has_back = 0;
						break;
				}
			}
		}	break;
	}
}

static void
_sc_draw_store_esc(
	sc_add_context_t *add,
	sc_draw_t *s,
	uint8_t c)
{
	switch (c) {
		case 'c': // reset all
			sc_draw_clear(s);
			break;
	}
}

static void
_sc_draw_store_param_digit(
	sc_add_context_t *add,
	uint8_t c)
{
	if (!add->pcount) {
		add->p[0] = 0;
		add->pcount = 1;
	}
	add->p[add->pcount - 1] *= 10;
	add->p[add->pcount - 1] += c - '0';
}

// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

static inline unsigned int
stb_ttc__UTF8_Decode(
		unsigned int* state,
		unsigned int* codep,
		unsigned char byte)
{
	static const unsigned char utf8d[] = {
		// The first part of the table maps bytes to character classes that
		// to reduce the size of the transition table and create bitmasks.
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

		// The second part is a transition table that maps a combination
		// of a state of the automaton and a character class to a state.
		0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
		12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
		12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
		12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
		12,36,12,12,12,12,12,12,12,12,12,12,
	};
	unsigned int type = utf8d[byte];
	*codep = (*state != UTF8_ACCEPT) ?
				(byte & 0x3fu) | (*codep << 6) :
				(0xff >> type) & (byte);
	*state = utf8d[256 + *state + type];
	return *state;
}

static void
_sc_draw_store_machine(
	sc_add_context_t *add,
	sc_draw_t *s,
	uint8_t c)
{
	pt_start(add->pt);

	switch (c) {
		case 27: {
			do {
				pt_yield(add->pt);
				switch (c) {
					case '[': {
						add->pcount = 0;
						do {
							pt_yield(add->pt);
							switch (c) {
								case '0' ... '9':
									_sc_draw_store_param_digit(add, c);
									break;
								case ';':
									add->pcount++;
									add->p[add->pcount - 1] = 0;
									break;
								default:
									_sc_draw_store_csi(add, s, c);
									pt_finish(add->pt);	// restart machine
							}
						} while (1);
					}	break;
					case '0' ... '9':
						_sc_draw_store_param_digit(add, c);
						break;
					default:
						_sc_draw_store_esc(add, s, c);
						pt_finish(add->pt);	// restart machine
				}
			} while (1);
		}	break;
		case '\t':
			s->c_x = (s->c_x + 1) & ~3;
			break;
		case '\b':
			if (s->c_x) s->c_x--;
			break;
		case '\n':
		case '\r': {
			sc_line_t * l = &s->line.e[s->c_y];
			if (l->count > s->c_x)	// trim line
				l->count = s->c_x;
			s->c_x = 0;
			if (c == '\n')
				s->c_y++;
		}	break;
		default:
			if (stb_ttc__UTF8_Decode(&add->utf8_state, &add->utf8_glyph, c) == UTF8_ACCEPT)
				_sc_draw_store_add(s, add->utf8_glyph, 0);
	}
	pt_end(add->pt);
}

//! like sc_add but using a add context and a sc_draw ('add' is optional)
int
sc_draw_add(
	sc_add_context_t *add,
	sc_draw_t *s,
	const char *what,
	unsigned int l)
{
	if (!what || !*what)
		return 0;
	if (!l) l = strlen(what);
	sc_add_context_t z = {};
	if (!add) add = &z;
	for (int ci = 0; ci < l; ci++)
		_sc_draw_store_machine(add, s, what[ci]);
	return l;
}

int
sc_add(
	sc_t *sc,
	const char *what,
	unsigned int l)
{
	if (!what || !*what)
		return 0;
	if (!l) l = strlen(what);
	SC_GET(sc);
	sc_draw_add(&sc->add, &sc->current->draw, what, l);
	if (sc->current->draw.dirty)
		sc_win_dirty(sc->current);
	return l;
}

int
sc_printf(
	sc_t *sc,
	const char *f,
	...)
{
	SC_GET(sc);
	va_list ap;
	va_start(ap, f);
	int res = sc_draw_vprintf(&sc->add,
					&sc->current->draw, f, ap);
	va_end(ap);
	if (sc->current->draw.dirty)
		sc_win_dirty(sc->current);
	return res;
}
// #include "sc_draw.c"


void
sc_draw_clear(
		sc_draw_t * s)
{
	for (int i = 0; i < s->line.count; i++)
		sc_line_free(&s->line.e[i]);
	sc_lines_free(&s->line);
	s->style.raw = 0;
	s->c_x = s->c_y = 0;
}

void
sc_draw_dirty(
		sc_draw_t * s)
{
	s->dirty = 1;
}

void
sc_draw_goto(
	sc_draw_t *s,
	int x, int y)
{
	if (x >= 0)
		s->c_x = x;
	if (y >= 0)
		s->c_y = y;
}

void
sc_draw_dispose(
		sc_draw_t *s)
{
	for (int li = 0; li < s->line.count; li++)
		sc_line_free(&s->line.e[li]);
	sc_lines_free(&s->line);
}

int
sc_draw_vprintf(
	sc_add_context_t *add,
	sc_draw_t *s,
	const char *f,
	va_list ap)
{
	sc_buf_t b = {};
	int res = sc_buf_vprintf(&b, f, ap);
	sc_add_context_t z = {};
	if (!add) add = &z;
	res = sc_draw_add(add, s, (char*)b.e, b.count);
	sc_buf_free(&b);
	return res;
}

int
sc_draw_printf(
	sc_add_context_t *add,
	sc_draw_t *s,
	const char *f,
	...)
{
	va_list ap;
	va_start(ap, f);
	int res = sc_draw_vprintf(add, s, f, ap);
	va_end(ap);
	return res;
}
// #include "sc_box.c"

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
	sc_win_t *_s,
	const sc_box_style_t * style,
	const int x, int y, int w, int h )
{
	if (w < 3 || h < 3) return -1;

	sc_draw_t * s = &_s->draw;
	sc_draw_goto(s, x, y);
	_sc_draw_store_add(s, style->tl, 0);
	for (int sx = x+1; sx < (x + w - 1); sx++)
		_sc_draw_store_add(s, style->h, 0);
	_sc_draw_store_add(s, style->tr, 0);
	sc_draw_goto(s, x, y + h-1);
	_sc_draw_store_add(s, style->bl, 0);
	for (int sx = x+1; sx < (x + w - 1); sx++)
		_sc_draw_store_add(s, style->h, 0);
	_sc_draw_store_add(s, style->br, 0);

	for (int sy = y + 1; sy < (y + h - 1); sy++) {
		sc_draw_goto(s, x, sy);
		_sc_draw_store_add(s, style->v, 0);
		sc_draw_goto(s, x + w - 1, sy);
		_sc_draw_store_add(s, style->v, 0);
	}
	sc_win_dirty(_s);
	return 0;
}

static void
_sc_box_render_cb(
	sc_win_t *s)
{
	sc_box_t * box = (sc_box_t*)s;
	_sc_box_render(s, &sc_box_style[s->draw.draw_style], 
			0, 0, s->draw.w, s->draw.h);
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
	s->x = x; s->y = y; s->draw.w = w; s->draw.h = h;
	s->driver = &_driver;
	s->draw.draw_style = flags;
	s->draw.kind = SC_BOX_PLAIN;

	sc_win_t *sub = sc_win_new(sc, s, 0);
	sub->x = 1; sub->y = 1; sub->draw.w = w - 2; sub->draw.h = h - 2;
	sc->current = sub;

	return sub;
}

int
sc_box_title_set(
	sc_win_t *box_in,
	const char *title)
{
	if (!box_in || !box_in->parent ||
			box_in->parent->draw.kind != SC_BOX_PLAIN)
		return -1;
	sc_box_t * box = (sc_box_t*)box_in->parent;

	if (box->title)
		free(box->title);
	if (title)
		box->title = strdup(title);
	return 0;
}

// #include "sc_render.c"

static char *
_sc_render_utf8_glyph(
	char * dst,
	unsigned int uni_glyph )
{
	if (uni_glyph < 128) {
		*dst = (char)uni_glyph;
		return dst + 1;	// that was easy
	}
	unsigned char *cur = (unsigned char*)dst;

	int cnt = 0;
	unsigned int mask = ~0x3f;
	while (uni_glyph & mask) {
		mask <<= 6;
		cnt++;
	}
	if ((uni_glyph >> (cnt *6)) & (0xf80 >> (cnt+1)))
		cnt++;
	unsigned char h = 0xf80 >> cnt;
	unsigned char hd = uni_glyph >> (cnt * 6);
	*cur++ = h | hd;
	while (cnt > 0) {
		cnt--;
		*cur++ = 0x80 | ((unsigned char)(uni_glyph >> (cnt * 6)) & 0x3f);
	}
	return (char*)cur;
}

/*
 * render a draw 's' into the parent 'd' at x,y
 * this is the 'blitter' really
 */
static void
_sc_draw_render(
	sc_draw_t *dst,
	sc_draw_t *src,
	int dst_x, int dst_y )
{
	for (int y = 0; y < src->h; y++) {
		int dy = dst_y + y;
		if (dy < 0 || dy >= dst->h)
			continue;
		if (y < src->line.count) {	// non-empty line
			sc_line_t * l = &src->line.e[y];
			int dx = dst_x, sx = 0;
			#if 0
			int leading = 0;// count leading spaces, for center/right justify
			for (int i = 0; i < l->count; i++)
				if (l->e[i].g == 0 || l->e[i].g == ' ')
					leading++;
			#endif
			switch (src->justify) {
				case SC_WIN_JUSTIFY_CENTER:
					if (l->count > src->w)
						sx = (l->count - src->w) / 2;
					else
						dx += (src->w - l->count) / 2;
					break;
				case SC_WIN_JUSTIFY_RIGHT:
					if (l->count > src->w)
						sx = l->count - src->w;
					else
						dx += src->w - l->count;
					break;
				case SC_WIN_JUSTIFY_LEFT:
				default:
					break;
			}
			for (; dx < (dst_x + src->w) && sx < l->count; sx++, dx++) {
				if ((dx >= 0 && dx < dst->w)) {
					sc_draw_store_xy(dst, &l->e[sx], dx, dy);
				}
			}
			for (; dx < (dst_x + src->w); dx++) {
				if ((dx >= 0 && dx < dst->w)) {
					sc_draw_store_xy(dst, NULL, dx, dy);
				}
			}
		} else {	// empty line, clear it.
			for (int x = 0; x < src->w; x++) {
				int dx = dst_x + x;
				if ((dx >= 0 && dx < dst->w)) {
					sc_draw_store_xy(dst, NULL, dx, dy);
				}
			}
		}
	}
}

static void
_sc_render_subs(
	sc_win_t *s,
	int force)
{
	sc_win_t *sb;
	if (!s->draw.dirty) {
		TAILQ_FOREACH_REVERSE(sb, &s->sub, sub, self) {
			if (sb->draw.dirty) {
				s->draw.dirty = 1;
				break;
			}
		}
	}
	if (!s->draw.dirty)
		return;
	s->draw.dirty = 0;
	TAILQ_FOREACH_REVERSE(sb, &s->sub, sub, self) {
		_sc_render_subs(sb, 0);
	}
	if (!s->parent)
		return;
	if (s->driver && s->driver->draw)
		s->driver->draw(s);

	_sc_draw_render(&s->parent->draw, &s->draw, s->x, s->y);
}

static void
_sc_render_flush_style(
	sc_t * sc,
	sc_buf_t * out)
{
	if (sc->render.style_count == 0)
		return;
	sc_buf_t seq = {};
	sc_style_t n = sc->render.style;
	sc_style_t o = sc->render.old_style;
#if 0
	printf("%s offset %d count %d old %x new %x\n", __func__,
			sc->render.style_insert,
			sc->render.style_count,
			o.raw, n.raw);
#endif
	sc->render.old_style = sc->render.style;
	sc->render.style_count = 0;

	int c = 0;
	sc_buf_concat(&seq, (uint8_t*)"\033[", 0);
	if (n.bold != o.bold)
		sc_buf_printf(&seq, "%s%d", c++ > 0 ? ";" : "",  n.bold ? 1 : 22);
	if (n.under != o.under)
		sc_buf_printf(&seq, "%s%d", c++ > 0 ? ";" : "", n.under ? 4 : 24);
	if (n.invert != o.invert)
		sc_buf_printf(&seq, "%s%d", c++ > 0 ? ";" : "", n.invert ? 7 : 27);
	if (n.fore != o.fore) {
		if (!n.has_fore)
			sc_buf_printf(&seq, "%s0", c++ > 0 ? ";" : "");
		else if (n.fore < 8)
			sc_buf_printf(&seq, "%s%d", c++ > 0 ? ";" : "", 30 + n.fore);
		else
			sc_buf_printf(&seq, "%s38;5;%d", c++ > 0 ? ";" : "", n.fore);
	}
	if (n.back != o.back) {
		if (!n.has_back)
			sc_buf_printf(&seq, "%s0", c++ > 0 ? ";" : "");
		else if (n.back < 8)
			sc_buf_printf(&seq, "%s%d", c++ > 0 ? ";" : "", 40 + n.back);
		else
			sc_buf_printf(&seq, "%s48;5;%d", c++ > 0 ? ";" : "", n.back);
	}
	if (!c)
		sc_buf_add(&seq, '0');
	sc_buf_add(&seq, 'm');
#if 0
	printf("     Insert offset %d, %d bytes seq: ",
		   sc->render.style_insert, seq.count);
	for (int i = 0; i < seq.count; i++) {
		if (seq.e[i] < ' ')
			printf("%02x", seq.e[i]);
		else printf("%c", seq.e[i]);
	}
	printf("\n");
#endif
	sc_buf_insert(out, sc->render.style_insert, seq.e, seq.count);
	sc_buf_free(&seq);
}

static void
_sc_render_flush_space(
	sc_t * sc,
	sc_buf_t * out)
{
	if (sc->render.space_count > 4) {
		//	printf("HT %d\n", sc->render.space_count);
		sc_buf_printf(out, "\033[%dC", sc->render.space_count);
	} else
		for (int si = 0; si < sc->render.space_count; si++)
			sc_buf_add(out, ' ');
	sc->render.space_count = 0;
}

static void
_sc_render_glyph(
	sc_t *sc,
	sc_buf_t * o,
	sc_line_t *l,
	unsigned int x)
{
	sc_glyph_t *g = &l->e[x];

	/*
	 * Count the characters we output with the same style. If the style change,
	 * go back and insert the style setting sequence at the last point we
	 * detected a change.
	 */
	if (g->style.raw != sc->render.style.raw) {
		_sc_render_flush_space(sc, o);
		_sc_render_flush_style(sc, o);
		// next insert point for a style change
		sc->render.style_insert = o->count;
		sc->render.style_count = 1;
		sc->render.style.raw = g->style.raw;
	} else
		sc->render.style_count++;
	// if we had a space run, and find something else, flush spaces.
	switch (g->g) {
		/*
		 * We count space, don't store them; if a run is bigger than 5,
		 * it's worth using the "move cursor" sequence. That only works if
		 * there are no style change of course
		 */
		case 0: // default/space
		case ' ':
			sc->render.space_count++;
			break;
		default: {
			if (sc->render.space_count)
				_sc_render_flush_space(sc, o);
			if (g->g > 0x7f) {
				char ut[8];
				int len = _sc_render_utf8_glyph(ut, g->g) - ut;
				sc_buf_concat(o, (uint8_t*)ut, len);
			} else
				sc_buf_add(o, g->g);
		}	break;
	}
}

static void
_sc_render_line(
	sc_t *sc,
	sc_win_t *s,
	sc_buf_t * o,
	sc_line_t *l )
{
	sc_buf_concat(o, (uint8_t*)"\033[0m", 0);	// CLREOL
	if (l->count) {
		sc_buf_concat(o, (uint8_t*)"\033[K", 0);	// CLREOL
	}
	sc->render.pt = NULL;
	sc->render.style.raw = 0;
	sc->render.style_count = 0;
	sc->render.space_count = 0;
	for (int x = 0; x < l->count; x++) {
		_sc_render_glyph(sc, o, l, x);
	}
	if (sc->render.style.raw != sc->render.old_style.raw)
		_sc_render_flush_style(sc, o);
}

int
sc_render(
	sc_t *sc,
	uint8_t flags)
{
	SC_GET(sc);
	sc_win_t *s = &sc->screen;

	_sc_render_subs(s, 0);

	sc_buf_free(&sc->output);
	if (sc->output.lines) {
		sc_buf_printf(&sc->output, "\033[%dA", sc->output.lines);
	}
	sc_buf_push(&sc->output, '\r');
#ifdef DEBUG
	if (sc->output.lines) {	// debug
		sc_buf_printf(&sc->output, "%d lines. Term %dx%d\r",
					  sc->output.lines, sc->screen.w, sc->screen.h);
	}
#endif
	sc_draw_t *d = &s->draw;
	sc->output.lines = 0;
	for (int y = 0; y < d->line.count; y++) {
		sc_line_t * l = &d->line.e[y];
		_sc_render_line(sc, s, &sc->output, l);
		sc_buf_push(&sc->output, '\n');
		sc->output.lines++;
	}
	//printf("\033[0mOut is %d bytes\n", sc->output.count);
	if (!(flags & SC_RENDER_NO_STDOUT))
		fwrite(sc->output.e, sc->output.count, 1, stdout);
	return 0;
}
// #include "sc_win.c"

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
// #include "sc_base.c"
#include <sys/ioctl.h>

sc_t * g_sc = NULL;

sc_t *
sc_new(
	uint32_t flags)
{
	sc_t *sc = calloc(1, sizeof(*sc));
	sc_win_t * s = &sc->screen;
	_sc_win_init(sc, NULL, s);
	sc->current = s;
	sc->flags = flags;

	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	s->draw.w = w.ws_col;
	s->draw.h = w.ws_row;
	if (!g_sc) g_sc = sc;
	return sc;
}

void
sc_dispose(
	sc_t * sc)
{
	if (!sc)
		sc = g_sc;
	if (!sc)
		return;
	sc_win_dispose(&sc->screen);
	sc_buf_free(&sc->output);
	if (g_sc == sc)
		g_sc = NULL;
	free(sc);
}

void
sc_goto(
	sc_t *sc, int x, int y)
{
	SC_GET(sc);
	sc_win_t *s = sc->current;
	sc_win_goto(s, x, y);
}

#include <termios.h>
#include <unistd.h>
#include <poll.h>

unsigned int
sc_getch(
	sc_t * sc,
	unsigned int timeout_ms)
{
	struct termios oldt, newt;
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );
	if (timeout_ms) {
		struct pollfd fd = {.fd = 0, .events = POLLIN };
		int r = poll(&fd, 1, timeout_ms);
		tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
		if (r <= 0)
			return 0;
	}
	uint8_t buf[8];
	int rd = read(0, buf, sizeof(buf));
	unsigned int ch = 0;
	for (int i = 0; i < rd; i++)
		ch = (ch << 8) | buf[i];
	return ch;
}

#endif /* SC_H_IMPLEMENTATION */
