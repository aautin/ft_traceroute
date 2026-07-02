#include <stdio.h>

void announceHelp()
{
	printf("Usage: ./ft_traceroute [options] <host>\n");
	printf("Options:\n");
	printf("  -? --help          Show this help message\n");
	printf("  -N --sim-queries   Set the number of simultaneous queries (default: 16)\n");
	printf("  -m --max-hops      Set the maximum number of hops (default: 30)\n");
	printf("  -p --port          Set the destination port (default: 33434)\n");
	printf("  -q --queries       Set the number of queries per hop (default: 3)\n");
	printf("  -w --wait          Set the wait time in seconds (default: 5)\n");
}
