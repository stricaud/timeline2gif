#include <stdio.h>
#include "t2g.h"
#include "output.h"
#include "timeline2gif.h"
#include <parser.h>

/* Parse `input_tig`. If `basedir` is non-NULL, relative image paths resolve
   against it (used by the GUI, whose edited text lives in a temp file);
   otherwise they resolve against the input file's own directory. */
static t2g_t *parse_tig_in(const char *input_tig, const char *basedir)
{
	FILE *in = fopen(input_tig, "r");
	if (!in) {
		fprintf(stderr, "Cannot open %s: ", input_tig);
		perror(NULL);
		return NULL;
	}
	t2g_t *t2g = t2g_new();
	if (basedir) t2g_set_basedir_dir(basedir);
	else         t2g_set_basedir(input_tig);
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

int t2g_generate_in(const char *input_tig, const char *output_path,
                    const char *basedir)
{
	t2g_t *t2g = parse_tig_in(input_tig, basedir);
	if (!t2g) return 1;
	int ret = write_output(t2g, output_path);
	t2g_free(t2g);
	return ret;
}

int t2g_generate_frames_in(const char *input_tig, t2g_frame_list_t *out,
                           const char *basedir)
{
	t2g_t *t2g = parse_tig_in(input_tig, basedir);
	if (!t2g) return 1;
	int ret = write_frames(t2g, out);
	t2g_free(t2g);
	return ret;
}

int t2g_generate(const char *input_tig, const char *output_path)
{
	return t2g_generate_in(input_tig, output_path, NULL);
}

int t2g_generate_frames(const char *input_tig, t2g_frame_list_t *out)
{
	return t2g_generate_frames_in(input_tig, out, NULL);
}
