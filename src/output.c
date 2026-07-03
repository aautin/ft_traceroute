#include "ft_traceroute.h"

#include <stdio.h>

static t_outputStatus outputStatus = { .hopIndex = 0, .queryIndex = 0 };

t_outputStatus getOutputStatus()
{
	return outputStatus;
}

void announceHelp()
{
	printf("Usage: ./ft_traceroute [options] <host>\n");
	printf("Options:\n");
	printf("  -? --help          Show this help message\n");
	printf("  -N --sim-queries   Set the number of simultaneous queries (default: 16)\n");
	printf("  -m --max-hops      Set the maximum number of hops (default: 30)\n");
	printf("  -p --port          Set the destination port (default: 33434)\n");
	printf("  -q --queries       Set the number of queries per hop (default: 3)\n");
	printf("  -w --wait          Set the MAX wait time (default: 5.0), \
\n%24s the HERE factor (default: 3)\n%24s the NEAR factor (default: 10)\n", "", "");
}

void announceOptions(t_options *options)
{
	printf("Options:\n");
	printf("  -N --sim-queries   %u\n", options->simQueries);
	printf("  -m --max-hops      %u\n", options->maxHops);
	printf("  -p --port          %u\n", options->port);
	printf("  -q --queries       %u\n", options->queries);
	printf("  -w --wait          %.6f, %u, %u\n",
		(double)options->wait.max.tv_sec + (double)options->wait.max.tv_usec / 1e6,
		options->wait.here,
		options->wait.near);
}
