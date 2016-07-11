#include <cstdio>

#include "mission/mission.hpp"

/**
	Runs mission.

	Mission writes commands to stdout and
	reads information from stdin by default.

	@param argv[1] the config file.
		If not supplied, uses default config.
	@param argv[2,3] the input and output files/pipes.
		If not supplied, uses stdin and stdout.
*/
int main(int argc, char** argv)
{
	FILE* in = stdin;
	FILE* out = stdout;
	FILE* config;

	if (argc < 2) config = NULL;
	else
	{
		config = fopen(argv[1], "r");
		if (!config)
		{
			fprintf(stderr, "error: unable to open config file\n");
			return 1;
		}
	}

	if (argc >= 4)
	{
		in = fopen(argv[2], "r");
		if (!in)
		{
			fprintf(stderr, "error: unable to open input file\n");
			return 1;
		}
		out = fopen(argv[3], "r");
		if (!out)
		{
			fprintf(stderr, "error: unable to open output file\n");
			return 1;
		}
	}
	return mission(in, out, config);
}

