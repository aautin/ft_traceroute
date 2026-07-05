#include "ft_traceroute.h"

void readReplies(t_context *context)
{
	//
	// To be continued
	//
}

void sendNextProbe(t_context *context, uint32_t nextIndex)
{
	uint8_t queries = context->options.queries;
	//
	// TTL
	//
	int ttl = (nextIndex / queries) + 1;
	setsockopt(context->udpSocket, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

	
	//
	// Destination address
	//
	struct sockaddr_in dest = {0};
	dest.sin_family = AF_INET;
	dest.sin_port = htons(context->options.port + nextIndex);
	inet_pton(AF_INET, context->host.ipName, &dest.sin_addr);

	//
	// Send the UDP packet with a fixed payload of 32 bytes
	//
	uint8_t payload[32] = {0};
	if (sendto(context->udpSocket, payload, sizeof(payload), 0,
		(struct sockaddr *)&dest, sizeof(dest)) == -1)
	{
		fatalError("sendto()");
	}

	//
	// Set probe port, send time and status
	//
	t_probe *probe = &context->rounds[ttl - 1].probes[nextIndex % queries];
	probe->port = context->options.port + nextIndex;
	gettimeofday(&probe->sendTime, NULL);
	probe->status = WAITING_FOR_REPLY;
}
