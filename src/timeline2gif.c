#include <stdio.h>
#include "t2g.h"
#include "output.h"
#include <parser.h>

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s timeline.tig output.gif|.webp|.apng\n",
		        argv[0]);
		return 1;
	}

	FILE *in = fopen(argv[1], "r");
	if (!in) {
		fprintf(stderr, "Cannot open %s: ", argv[1]);
		perror(NULL);
		return 1;
	}

	t2g_t *t2g = t2g_new();

	t2g_set_basedir(argv[1]);
	t2g_parser_init();
	t2grestart(in);
	t2gparse(t2g);
	t2g_parser_terminate();

	fclose(in);

	int ret = write_output(t2g, argv[2]);

	t2g_free(t2g);
	return ret;
}
