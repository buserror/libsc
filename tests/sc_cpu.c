/*
 * sc_cpu.c
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _GNU_SOURCE // for asprintf
#include <stdio.h>
#ifndef DEBUG
#define SC_H_IMPLEMENTATION
#endif
#include "sc.h"
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <sys/sysinfo.h>
#include <termios.h>
#include <glob.h>

typedef struct s_core_t {
	long long cpu, idle;
	long usage;
	int peak : 7, awake : 1, phy : 8, core : 8, proc: 8;
} s_core_t;
DECLARE_C_ARRAY(s_core_t, core, 2);
DECLARE_C_ARRAY(s_core_t*, thread, 2);
typedef struct s_phy_t {
	char * model;
	int phy : 4;
	int coretemp_fd;
	char temp_label[16];
	core_t core;
} s_phy_t;
DECLARE_C_ARRAY(s_phy_t, phy, 2);

IMPLEMENT_C_ARRAY(core);
IMPLEMENT_C_ARRAY(thread);
IMPLEMENT_C_ARRAY(phy);

static int read_sys_file(int fd, char *dst, int dst_size)
{
	if (fd <= 0 || !dst || !dst_size)
		return 0;
	dst[0] = 0;
	lseek(fd, SEEK_SET, 0);
	int s = read(fd, dst, dst_size-1);
	if (s == -1 || s == dst_size-1) {
		perror("sys file read error");
		exit(1);
	}
	while (s > 0 && dst[s-1] < ' ') dst[--s] = 0;
	return s;
}

static int open_sys_file(
		const char *base, const char *fname, int keep_open,
		char *dst, int dst_size)
{
	char * full;
	asprintf(&full, "%s%s%s", base, fname ? "/" : "", fname ? fname : "");
	int fd = open(full, O_RDONLY);
	free(full);
	if (fd == -1) return -1;
	int s = 0;
	if (dst && dst_size)
		s = read_sys_file(fd, dst, dst_size);
	if (!keep_open) close(fd);
	return keep_open ? fd : s;
}

int main(int argc, const char *argv[])
{
	int sort = 0;	// sort CPU per usage
	char buf[8192];
	phy_t p = {};
	thread_t cpu = {};

	thread_realloc(&cpu, sysconf(_SC_NPROCESSORS_ONLN));
	cpu.count = cpu.size;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			fprintf(stderr, "%s: --help\n", argv[0]);
			fprintf(stderr, "	version %s\n", BUILT);
			exit(0);
		} else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--sort"))
			sort++;
	}
	{	// map the threads numbers back to processor/core and make a table
		s_phy_t *c_p = NULL;
		int proc_id = 0;
		char model[128];

		FILE *f = fopen("/proc/cpuinfo", "r");
		while (fgets(buf, sizeof(buf), f)) {
			if (!strncmp(buf, "processor", 9)) {
				char * col = strchr(buf + 9, ':');
				proc_id = atoi(col + 2);
			} else if (!strncmp(buf, "physical id", 11)) {
				char * col = strchr(buf + 11, ':');
				int v = atoi(col + 2);
				while (v >= p.count) {
					s_phy_t z = { .phy = v };
					z.model = strdup(model);
					phy_add(&p, z);
				}
				c_p = &p.e[v];
			} else if (!strncmp(buf, "core id", 7)) {
				char * col = strchr(buf + 7, ':');
				int v = atoi(col + 2);
				s_core_t z = { .phy = c_p->phy, .proc = proc_id, .core = v };
				core_add(&c_p->core, z);
			} else if (!strncmp(buf, "model name", 10)) {
				char * s = strchr(buf + 10, ':') + 2;
				char *start = s;
				char * skip;
				if ((skip = strstr(start, "CPU ")) != NULL)
					memmove(skip, skip + 4, strlen(skip) - 3);
				if ((skip = strstr(start, "Intel(R) ")) != NULL)
					memmove(skip, skip + 9, strlen(skip) - 8);
				while (strlen(start) && start[strlen(start)-1] < ' ')
					start[strlen(start)-1] = 0;
				strncpy(model, start, sizeof(model)-1);
			}
		}
		fclose(f);
	}
	// initialises the cpu thread ID -> core/threads table
	for (int pi = 0; pi < p.count; pi++) {
		s_phy_t * phy = &p.e[pi];
	//	printf("%d: %s\n", phy->phy, phy->model);
		for (int ti = 0; ti < phy->core.count; ti++) {
			s_core_t * t = &phy->core.e[ti];
		//	printf("  proc %d core %d:%d\n", t->proc, t->phy, t->core);
			cpu.e[t->proc] = t;
		}
	}
	{	// search for a coretemp module, and get the package input one
		glob_t g = {};
		if (glob("/sys/class/hwmon/hwmon*", 0, NULL, &g) == 0) {
			for (int i = 0; i < g.gl_pathc; i++) {
				char buf[32] = "";
				open_sys_file(g.gl_pathv[i], "name", 0, buf, sizeof(buf));
				if (strncmp(buf, "coretemp", 8)) continue;
				open_sys_file(g.gl_pathv[i], "temp1_label", 0, buf, sizeof(buf));
				if (strncmp(buf, "Package id ", 11)) continue;
				int pi = atoi(buf+11);
				if (pi < p.count) {
					p.e[pi].coretemp_fd = open_sys_file(
						g.gl_pathv[i], "temp1_input", 1, buf, sizeof(buf));
				//	printf("coretemp fd %d current %s\n", p.e[pi].coretemp, buf);
				}
			}
			globfree(&g);
		}
	}
	int fd = open("/proc/stat", O_RDONLY);
	if (fd == -1) {
		perror("/proc/stat");
		exit(1);
	}
	int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
	if (timerfd == -1) {
		perror("timerfd_create(CLOCK_MONOTONIC) failed");
		exit(1);
	} else {
		struct itimerspec timerconfig = {
			.it_interval.tv_nsec = 1000000000 / 4,	// 250ms
			.it_value.tv_sec = 1,
		};
		timerfd_settime(timerfd, 0, &timerconfig, NULL);
	}
	struct termios oldt;	// save terminal state
	tcgetattr(0, &oldt);
	sc_new(SC_PARTIAL | SC_REDRAW);
	int tw_width = 0, th_width = 1;	// number of char cols per threads
	int th_height = 10, th_spacer = 1, quit = 0;
	switch (cpu.count) {
		case 1 ...   4: th_width = 8; th_spacer = 1; break;
		case 5  ... 12:	th_width = 4; th_spacer = 1; break;
		case 13 ... 24:	th_width = 3; th_spacer = 1; break;
		case 25 ... 40: th_width = 2; th_spacer = 0; break;
		default:  		th_width = 1; th_spacer = 0; break;
	}
	tw_width = (cpu.count * th_width) + ((cpu.count - 1) * th_spacer);
	sc_win_t * chart = sc_box(NULL, 0, 0, tw_width + 2, th_height + 2, 0);
	sc_box_title_set(chart, "Press Q to quit");
	int ticks = 0, set_title = 0;
	char model[128] = "Press Q to quit";
	char title[256] = "";
	do {
		if (ticks == 4*5 && p.e[0].model) {
			if (p.count > 1)
				sprintf(model, "%d x %s", p.count, p.e[0].model);
			else
				strncpy(model, p.e[0].model, sizeof(model));
			set_title++;
		}
		if (!(ticks & 7)) {	// read temperatures every 2 seconds
			for (int pi = 0; pi < p.count; pi++) if (p.e[pi].coretemp_fd) {
				char b[32];
				read_sys_file(p.e[pi].coretemp_fd, b, sizeof(b));
				int c = (atoi(b) + 500) / 1000;
				sprintf(p.e[pi].temp_label, "%dC", c);
			}
			set_title++;
		}
		if (set_title) {
			set_title = 0;
			strncpy(title, model, sizeof(title));
			/* you can add (SOME!) cursor movement labels to the title */
			for (int ci = 0; ci < p.count; ci++)
				if (p.e[ci].temp_label[0])
					sprintf(title + strlen(title), "\033[%dG%s",
							tw_width - 2 - ((p.count - 1 - ci) * 4),
							p.e[ci].temp_label);
			sc_box_title_set(chart, title);
		}
		read_sys_file(fd, buf, sizeof(buf));// read /proc/stats

		const int colors[] = { 0, 17, 53, 89, 125, 161, 197 };

		for (int ci = 0; ci < cpu.count; ci++)
			cpu.e[ci]->awake = 0;
		read_sys_file(fd, buf, sizeof(buf));// read /proc/stats
		char *sep = buf;
		char *line;
		while ((line = strsep(&sep, "\n")) != NULL) {
			if (line[0] != 'c' || line[1] != 'p' || line[2] != 'u') break;
			if (line[3] == ' ') continue;	// total cpu usage, don't care
			int cid = atoi(line + 3);
			int idx = 0;

			s_core_t * now = cpu.e[cid];
			long long idle = now->idle, cpu = now->cpu;
			now->awake = 1;
			now->cpu = now->idle = 0;
			for (char *field = strsep(&line, " "); field;
						field = strsep(&line, " "), idx++) {
				if (idx == 0)
					continue;
				long long v = atol(field);
				now->cpu += v;
				if (idx == 4)
					now->idle = v;
			}
			now->usage = 100 - (((now->idle - idle) * 100.0) /
										(now->cpu - cpu));
			if (now->usage < 0)
				now->usage = 0;
			if (now->usage >= 90)
				now->peak = 6;
		}
		sc_win_dirty(chart);	// force redraw
		int bx = 0;
		if (ticks) for (int pi = 0; pi < p.count; pi++) {
			s_phy_t * phy = &p.e[pi];
			for (int ti = 0; ti < phy->core.count; ti++) {
				s_core_t *t = &phy->core.e[ti];
				for (int y = 1; y <= th_height; y++) {
					int py = 100 - ((y * 100) / th_height);

					sc_glyph_t *g = sc_draw_store_xy(&chart->draw, NULL, bx, y-1);
					//	g->g = 0;
					if (py <= t->usage) {
						if (!t->awake)
							g->g = 0;
						else if ((t->usage - py) < 10) {
							g->g = 0x2581 + (((t->usage - py) * 8) / 10);
						} else
							g->g = 0x2588;	// one eight's, add up to 2588
					} else {	// peak drawing
						if (y == 1) {
							if (g->g >= 0x2581 && g->g <= 0x2588) {
								static const int map8to6[] = {0,1,2,3,4,4,5,5};
								g->g = 0x1FB7b - map8to6[g->g - 0x2581];
							}
						} else
							g->g = 0;
					}
					// color decay
					if (y == 1) {
						if (t->peak) {
							g->style.has_fore = 1;
							g->style.fore = colors[t->peak];
							t->peak--;
						} else if (g->style.has_fore) {
							g->style.has_fore = 0;
							g->style.fore = 0;
							g->g = 0;
						}
					}
					if (g) for (int x = 1; x < th_width; x++)
						*(sc_draw_store_xy(&chart->draw, NULL, bx + x, y - 1)) = *g;
				}
				bx += th_width + th_spacer;
			}
			bx += 1;	// space between phy
		}
		sc_render(NULL, 0);

		struct termios newt = oldt;
		newt.c_lflag &= ~( ICANON | ECHO );
		tcsetattr(0, TCSANOW, &newt);

		struct pollfd fds[2] = {
			{ .fd = 0, .events = POLLIN },
			{ .fd = timerfd, .events = POLLIN },
		};
		/*int rp = */poll(fds, 2, 2000 /* 2 * timer timeout */);
		tcsetattr(0, TCSANOW, &oldt);
		if (fds[0].revents) {	// key pressed
			char k[16];	// can't use sc_getch here, as we need the timer fd
			int rk = read(0, k, sizeof(k));
			if (rk > 0) switch (k[0]) {
				case 'q': quit++; break;
				case 'D': {
					printf("Output is %d bytes\r", chart->sc->output.count);
				}	break;
			}
		}
		if (fds[1].revents) {	// timer expired
			uint64_t expire = 0;
			/* size_t tr = */ read(timerfd, &expire, sizeof(expire));
		}
		ticks++;
	} while (!quit);
	close(fd);
	sc_dispose(NULL);
}
