#include <stdio.h>
#include "t2g.h"
#include <parser.h>


int main(int argc, char **argv)
{
	if (argc < 3) {
 	        fprintf(stderr, "Usage: %s timeline.t2g timeline.gif\n", argv[0]);
		return 1;
	}

	return 0;
}
