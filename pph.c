#!/usr/bin/tcc -run
/*
 * pph.c
 *
 * Copyright (C) 2022 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 * This loads the 'base' header file, converts all #include that have
 * the right prefix to the file contents itself.
 * Basically it converts a bunch of files to a single .h
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int
subfile(const char * fname, FILE *out, int skip_header, int recurse) {
	FILE *f = fopen(fname, "r");
	printf("%s#include \"%s\"\n", f ? "// " : "", fname + 4);
	if (!f) return -1;
	char line[512];
	while (fgets(line, sizeof(line), f)) {
		if (skip_header && (!strncmp(line, "/*", 2) || !strncmp(line, " *", 2) ||
				!strncmp(line, "*/", 2) || line[0] < ' ')) // skip dup licences
			continue;
		if (strstr(line, "define SC_H_INTERNAL"))
			continue;
		skip_header = 0;
		if (!strncmp(line, "#include \"", 10)) {
			char fname[64] = "src/";
			strcat(fname, line + 10);
			while (*fname && (fname[strlen(fname)-1] == '\n' ||
					fname[strlen(fname)-1] == '"'))
				fname[strlen(fname)-1] = 0;
			if (recurse)
				subfile(fname, out, 1, 0) == 0;
			continue;
		}
		fwrite(line, 1, strlen(line), out);
	}
	return 0;
}

int main (int argc, char **argv) {
	subfile("src/sc_base.h", stdout, 0, 1);
}
