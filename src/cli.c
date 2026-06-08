#include <stdio.h>
#include "timeline2gif.h"

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s timeline.tig output.gif|.webp|.apng\n",
		        argv[0]);
		return 1;
	}
	return t2g_generate(argv[1], argv[2]);
}
