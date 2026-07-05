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

/// @brief True if there is no waiting probe or if we have reached the destination
///        for a whole round and there is no waiting probe in the previous rounds
static bool isFinished(const t_context* context)
{
	bool hasReachedDestination = false;

	for (uint8_t i = 0; i < context->options.maxHops; ++i)
	{
		for (uint8_t j = 0; j < context->options.queries; ++j)
		{
			if (context->rounds[i].probes[j].status == PORT_UNREACHABLE)
			{
				hasReachedDestination = true;
			}
			
			if (context->rounds[i].probes[j].status == WAITING_FOR_REPLY || context->rounds[i].probes[j].status == WAITING_TO_SEND)
			{
				return false;
			}
		}
		if (hasReachedDestination)
		{
			return true;
		}
	}
	return true;
}

static void traceroute(t_context *context)
{
	uint32_t nextProbeIndex = 0;
	while (1)
	{
		readReplies(context);
		applyTimeouts(context);
		updateOutput(context);

		if (isFinished(context) && isOutputUpdated(context->rounds, context->options))
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
