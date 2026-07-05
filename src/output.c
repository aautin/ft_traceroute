#include "ft_traceroute.h"

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netdb.h>

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

static const char *getReverseDnsName(const char *ipName, char *buffer, size_t bufferSize)
{
	struct sockaddr_storage ss;
	memset(&ss, 0, sizeof(ss));

	if (strchr(ipName, ':') != NULL)
	{
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&ss;
		addr6->sin6_family = AF_INET6;
		if (inet_pton(AF_INET6, ipName, &addr6->sin6_addr) != 1)
			return ipName;
		if (getnameinfo((struct sockaddr *)addr6, sizeof(*addr6), buffer, (socklen_t)bufferSize, NULL, 0, NI_NAMEREQD) == 0)
			return buffer;
	}
	else
	{
		struct sockaddr_in *addr4 = (struct sockaddr_in *)&ss;
		addr4->sin_family = AF_INET;
		if (inet_pton(AF_INET, ipName, &addr4->sin_addr) != 1)
			return ipName;
		if (getnameinfo((struct sockaddr *)addr4, sizeof(*addr4), buffer, (socklen_t)bufferSize, NULL, 0, NI_NAMEREQD) == 0)
			return buffer;
	}
	return ipName;
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
	return false;
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
		printf("%2u  ", roundI + 1);
	}

	//
	// Address
	//
	if (probe->status != TIMEOUT && !equalToLastAddress(rounds, roundI, queryI))
	{
		char reverseDns[NI_MAXHOST];
		const char *resolvedName = getReverseDnsName(probe->hopIpName, reverseDns, sizeof(reverseDns));
		if (strcmp(resolvedName, probe->hopIpName) != 0)
			printf("%s (%s) ", resolvedName, probe->hopIpName);
		else
			printf("%s ", probe->hopIpName);
	}

	#ifdef DEBUG
		printf("(%u) ", probe->status);
	#endif
	//
	// Status (+ optional transport time)
	//
	char* status = NULL;
	switch (probe->status)
	{
		case TIME_EXCEEDED:
			printf("%6.3f ms ", transport);
			break;
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

void updateOutput(const t_context *context, bool* noWait, bool* unreachRoundPrinted)
{
	*noWait = true;
	*unreachRoundPrinted = false;
	for (uint8_t i = gNext.roundI; i < context->options.maxHops; ++i)
	{
		for (uint8_t j = (i == gNext.roundI ? gNext.queryI : 0); j < context->options.queries; ++j)
		{
			t_probe* probe = &context->rounds[i].probes[j];
			if (probe->status == PORT_UNREACHABLE)
			{
				if (j == context->options.queries - 1)
				{
					printProbe(context->options.queries, context->rounds, i, j);
					*unreachRoundPrinted = true;
					return;
				}
			}
			if (probe->status == WAITING_TO_SEND || probe->status == WAITING_FOR_REPLY)
			{
				*noWait = false;
				return;
			}

			printProbe(context->options.queries, context->rounds, i, j);
			gNext.roundI = (j == context->options.queries - 1) ? i + 1 : i;
			gNext.queryI = (j == context->options.queries - 1) ? 0 : j + 1;
		}
	}
}

void outputHeader(const t_context *context)
{
	printf("traceroute to %s (%s), %u hops max, %lu byte packets\n",
		context->host.name, context->host.ipName, context->options.maxHops, 32 + sizeof(struct ip) + sizeof(struct udphdr));
}