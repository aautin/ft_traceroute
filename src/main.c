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

static void applyTimeouts(t_context *context)
{
	struct timeval now;
	gettimeofday(&now, NULL);

	for (uint8_t i = 0; i < context->options.maxHops; ++i)
	{
		struct timeval timeout = getTimeout(context->options, context->rounds, i);
		for (uint8_t j = 0; j < context->options.queries; ++j)
		{
			t_probe* probe = &context->rounds[i].probes[j];
			if (probe->status == WAITING_FOR_REPLY && isTimeout(now, probe->sendTime, timeout))
			{
				probe->status = TIMEOUT;
			}
		}
	}
}

static void traceroute(t_context *context)
{
	uint32_t nextProbeIndex = 0;
	while (1)
	{
		readReplies(context);
		applyTimeouts(context);
		updateOutput(context);

		if (isFinished(context->rounds, context->options) && isOutputUpdated(context->rounds, context->options))
		{
			break;
		}
		
		//
		// Send new probes if we have not reached the simultaneous limit
		//
		uint64_t waitingForReply = getWaitingForReplyNumber(context->rounds, context->options);
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
