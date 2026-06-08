#include <stdio.h>
#include "t2g.h"
#include "output.h"
#include "timeline2gif.h"
#include <parser.h>

static t2g_t *parse_tig(const char *input_tig)
{
	FILE *in = fopen(input_tig, "r");
	if (!in) {
		fprintf(stderr, "Cannot open %s: ", input_tig);
		perror(NULL);
		return NULL;
	}
	t2g_t *t2g = t2g_new();
	t2g_set_basedir(input_tig);
	t2g_parser_init();
	t2grestart(in);
	int parse_ok = (t2gparse(t2g) == 0);
	t2g_parser_terminate();
	fclose(in);
	if (!parse_ok) {
		t2g_free(t2g);
		return NULL;
	}
	return t2g;
}

int t2g_generate(const char *input_tig, const char *output_path)
{
	t2g_t *t2g = parse_tig(input_tig);
	if (!t2g) return 1;
	int ret = write_output(t2g, output_path);
	t2g_free(t2g);
	return ret;
}

int t2g_generate_frames(const char *input_tig, t2g_frame_list_t *out)
{
	t2g_t *t2g = parse_tig(input_tig);
	if (!t2g) return 1;
	int ret = write_frames(t2g, out);
	t2g_free(t2g);
	return ret;
}

