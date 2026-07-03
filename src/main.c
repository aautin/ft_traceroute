#include "ft_traceroute.h"

static void clear(t_context *context)
{
	for (uint8_t i = 0; i < context->options.maxHops; ++i)
	{
		free(context->rounds[i].probes);
	}
	free(context->rounds);

	close(context->icmpSocket);
	close(context->udpSocket);
}

static void traceroute(t_context *context)
{
	//
	// To be continued
	//
}

int main(int argc, char **argv)
{
	t_context context;

	int nextIndex = initOptions(&context.options, argc, argv);
	initHost(&context.host, nextIndex, argc, argv);
	initSockets(&context);
	initProbes(&context.rounds, &context.options);

	traceroute(&context);
	clear(&context);
}
