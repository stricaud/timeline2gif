#include <stdio.h>
#include <timeline2gif.h>
#include <parser.h>

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s timeline.tig timeline.gif");
		return 1;
	}

	return 0;
}
