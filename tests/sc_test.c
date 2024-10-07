/*
 * sc_test.c
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef DEBUG
#define SC_H_IMPLEMENTATION
#endif
#include "sc.h"
#include <stdio.h>

int main()
{
	sc_t * sc = sc_new(SC_PARTIAL | SC_REDRAW);
	sc_win_t * main = sc_win_get(NULL);
	{
		sc_win_t * box = sc_box(NULL, 15, 1, 18, 4, 0);
		box->draw.justify = SC_WIN_JUSTIFY_RIGHT;
		sc_printf(NULL, "Hello There\n\033[38;5;14mAnother Line\n");
		sc_win_set(NULL, main);
	}
	{
		sc_win_t * box = sc_box(NULL, 20, 4, 25, 3, SC_BOX_ROUND);
		box->draw.justify = SC_WIN_JUSTIFY_CENTER;
		sc_printf(NULL, "Testing \033[1mOne \033[22;7m Two \033[0m");
		sc_win_set(NULL, main);
	}
	sc_win_t * update = sc_box(NULL, 50, 1, 30, 4, 0);
	sc_printf(NULL, "Press a key (Q to quit)");
	if (1) { // test for consecutive block of same colour
		sc_win_goto(update, 1, 1);
		sc_printf(NULL, "\033[48;5;161mAbc\033[0m");
		sc_win_goto(update, 5, 1);
		sc_printf(NULL, "\033[48;5;161mDef\033[0m Ghi");
	}
	if (1) {
		sc_render(NULL, 0);
	} else {
		sc_render(NULL, SC_RENDER_NO_STDOUT);
		printf("\033[0m\nOut is %d\n", sc->output.count);
		FILE *o = fopen("dump.bin", "wb");
		fwrite(sc->output.e, sc->output.count, 1, o);
		fclose(o);
		sc_draw_dump(&main->draw);
		exit(0);
	}
	int done = 0;
	sc_win_set(NULL, update);
	do {
		unsigned int k = sc_getch(NULL, 1000);
		switch (k) {
			case 'q':
				done = 1;
				break;
			case 0:
				continue;
		}
		sc_win_clear(update);
		sc_printf(NULL, "Key '%x' was pressed\r", k);
		sc_render(NULL, 0);
	} while (!done);
	sc_dispose(NULL);
}
