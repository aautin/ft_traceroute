#include "ft_traceroute.h"

#include <stdio.h>

// -------------- Structures, Enums, Typedefs -------------- //
typedef struct s_outputStatus
{
	uint8_t roundI;
	uint8_t queryI;
} t_outputStatus;

enum e_outputMode
{
	NEW_ROUND,
	SAME_ROUND_NEW_ADDRESS,
	SAME_ROUND_SAME_ADDRESS,
};

// -------------- Global statics -------------- //
static t_outputStatus gNext = { .roundI = 0, .queryI = 0 };

// -------------- Functions -------------- //
bool isOutputUpdated(const t_rounds *rounds, const t_options options)
{
	for (uint8_t i = gNext.roundI; i < options.maxHops; ++i)
	{
		for (uint8_t j = (i == gNext.roundI ? gNext.queryI : 0); j < options.queries; ++j)
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

static float getTransportTimeMs(const struct timeval *send, const struct timeval *receive)
{
	return (float)(receive->tv_sec - send->tv_sec) * 1000.0f +
		   (float)(receive->tv_usec - send->tv_usec) / 1000.0f;
}

static bool equalToLastAddress(const t_rounds *rounds, uint8_t roundI, uint8_t queryI)
{
	//
	// Returnning true if there is no previous probe with an address
	//              or if the last is the same as the current one
	//

	t_probe*    probes     = rounds[roundI].probes;
	const char* newAddress = probes[queryI].hopIpName;

	while (queryI > 0)
	{
		--queryI;
		if (probes[queryI].status != TIMEOUT && !strcmp(probes[queryI].hopIpName, newAddress))
		{
			return true;
		}
	}
	return true;
}

static void printProbe(uint8_t queriesPerHop, const t_rounds* rounds, uint8_t roundI, uint8_t queryI)
{
	const t_probe* probe = &rounds[roundI].probes[queryI];
	const float    transport = getTransportTimeMs(&probe->sendTime, &probe->receiveTime);
	
	//
	// Round 
	//
	if (queryI == 0)
	{
		printf("%2u  ", rounds[roundI].ttl);
	}

	//
	// Address
	//
	if (probe->status !=  TIMEOUT && !equalToLastAddress(rounds, roundI, queryI))
	{
		printf("%s ", probe->hopIpName);
		return;
	}

	//
	// Status (+ optional transport time)
	//
	char* status = NULL;
	switch (probe->status)
	{
		case NETWORK_UNREACHABLE:
			status = "!N";
			break;
		case HOST_UNREACHABLE:
			status = "!H";
			break;
		case PROTOCOL_UNREACHABLE:
			status = "!P";
			break;
		case UNKNOWN_ERROR:
			status = "!?";
			break;
		case TIMEOUT:
			printf("* ");
			break;
		case PORT_UNREACHABLE:
			printf("%6.3f ms ", transport);
			break;
		default:
	}
	if (status)
	{
		printf("%6.3f ms %s ", transport, status);
	}

	//
	// End of round
	//
	if (queryI == queriesPerHop - 1)
	{
		printf("\n");
	}
}

void updateOutput(const t_context *context)
{
	for (uint8_t i = gNext.roundI; i < context->options.maxHops; ++i)
	{
		for (uint8_t j = (i == gNext.roundI ? gNext.queryI : 0); j < context->options.queries; ++j)
		{
			t_probe* probe = &context->rounds[i].probes[j];
			if (probe->status == WAITING_TO_SEND || probe->status == WAITING_FOR_REPLY)
			{
				return;
			}

			printProbe(context->options.queries, context->rounds, i, j);
			gNext.roundI = (j == context->options.queries - 1) ? i + 1 : i;
			gNext.queryI = (j == context->options.queries - 1) ? 0 : j + 1;
		}
	}
}
