/*
 * sc_base.c
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/ioctl.h>
#define SC_H_INTERNAL
#include "sc_base.h"

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
