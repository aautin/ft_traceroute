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
	uint32_t nextProbeIndex = 0;
	while (1)
	{
		readReplies(context);

		t_outputStatus outputStatus = getOutputStatus();
		uint64_t       waitingForReply = getWaitingForReplyNumber(context->rounds, context->options);
		//
		// Break if output is up to date and no probe are waiting for reply
		//

		//
		// Send new probes if we have not reached the simultaneous limit
		//
		while (waitingForReply < context->options.simQueries)
		{
			sendNextProbe(context, nextProbeIndex);
			nextProbeIndex++;
		}
	}
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
