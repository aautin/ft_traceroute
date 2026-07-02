#include "ft_traceroute.h"

int main(int argc, char **argv)
{
	t_context context;

	int nextIndex = initOptions(&context.options, argc, argv);
	initHost(&context.host, nextIndex, argc, argv);
	initSockets(&context);
}