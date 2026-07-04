#include "ft_traceroute.h"

#include <stdio.h>

// -------------- Structures, Enums, Typedefs -------------- //
typedef struct s_outputStatus
{
	uint8_t hopI;
	uint8_t queryI;
} t_outputStatus;

enum e_outputMode
{
	NEW_ROUND,
	SAME_ROUND_NEW_ADDRESS,
	SAME_ROUND_SAME_ADDRESS,
};

// -------------- Global statics -------------- //
static t_outputStatus gNext = { .hopI = 0, .queryI = 0 };

// -------------- Functions -------------- //
bool isOutputUpdated(const t_rounds *rounds, const t_options options)
{
	for (uint8_t i = gNext.hopI; i < options.maxHops; ++i)
	{
		for (uint8_t j = (i == gNext.hopI ? gNext.queryI : 0); j < options.queries; ++j)
		{
			enum e_status status = rounds[i].probes[j].status;
			if (status != WAITING_TO_SEND && status != WAITING_FOR_REPLY)
			{
				return false;
			}
		}
	}
	return true;
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
		options->wait.here, options->wait.near);
}

static void printProbe(const t_rounds* rounds, uint8_t hopI, uint8_t queryI)
{
	//
	// New round 
	//
	if (queryI == 0)
	{

		return;
	}

	//
	// Same round, TIMEOUT
	//
	if (rounds)

	//
	// Same round, new address
	//
	if (!strcmp(rounds[hopI].probes[queryI].statusIp, rounds[hopI].probes[queryI - 1].statusIp))
	{

		return;
	}

	//
	// Same round, same address
	//
	
	
}

void updateOutput(const t_context *context)
{
	for (uint8_t i = gNext.hopI; i < context->options.maxHops; ++i)
	{
		for (uint8_t j = (i == gNext.hopI ? gNext.queryI : 0); j < context->options.queries; ++j)
		{
			t_probe* probe = &context->rounds[i].probes[j];
			if (probe->status == WAITING_TO_SEND || probe->status == WAITING_FOR_REPLY)
			{
				return;
			}

			printProbe(probe, contextgetOutputMode(context->rounds, i, j));
			gNext.hopI = (j == context->options.queries - 1) ? i + 1 : i;
			gNext.queryI = (j == context->options.queries - 1) ? 0 : j + 1;
		}
	}
}
