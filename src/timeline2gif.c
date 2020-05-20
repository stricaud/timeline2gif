#include <stdio.h>
#include "t2g.h"
#include "write-gif.h"
#include <parser.h>
#include "effects.h"

int main(int argc, char **argv)
{
	FILE *in, *out;
	t2g_t *t2g;
	int retval;

	if (argc < 3) {
 	        fprintf(stderr, "Usage: %s timeline.t2g timeline.gif\n", argv[0]);
		return 1;
	}

	in = fopen(argv[1], "r");
	if (!in) {
		fprintf(stderr, "Cannot import file %s\n", argv[1]);
		return -1;
	}

	t2g = t2g_new();

	t2g_parser_init();
	t2grestart(in);
	retval = t2gparse(t2g);
	t2g_parser_terminate();
	
	fflush(in);
	fclose(in);
	in = NULL;

	write_gif(t2g, argv[2]);
	
	return 0;
}
