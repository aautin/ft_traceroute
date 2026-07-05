#include "ft_traceroute.h"

#include <stdio.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <errno.h>
#include <arpa/inet.h>

void readReplies(t_context *context)
{
	while (1)
	{
		uint8_t 		   buffer[65535];
		struct sockaddr_in from;
		socklen_t          fromlen = sizeof(from);

		ssize_t n = recvfrom(context->icmpSocket, buffer, sizeof(buffer),
						0, (struct sockaddr *)&from, &fromlen);
		if (n == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				//
				// No packet available right now
				//
				break;
			}
			fatalError("recvfrom()");
		}

		if (n < (ssize_t) (sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct iphdr) + sizeof(struct udphdr)))
		{
			//
			// Packet is too small to contain the expected headers
			//
			continue;
		}

		struct iphdr  * ip          = (struct iphdr *)buffer;
		struct icmphdr* icmp        = (struct icmphdr *)(buffer + ip->ihl * 4);
		uint8_t*        icmpPayload = (uint8_t *)(icmp + 1);
		struct iphdr*   origIp      = (struct iphdr *)icmpPayload;
		struct udphdr*  origUdp     = (struct udphdr *)(icmpPayload + origIp->ihl * 4);
		
		uint16_t origPort   = ntohs(origUdp->dest);
		uint32_t probeIndex = origPort - context->options.port;
		uint32_t roundI     = probeIndex / context->options.queries;
		uint32_t probeI     = probeIndex % context->options.queries;
		
		//
		// Set probe receive time, status and hop IP name
		//
		t_probe *probe = &context->rounds[roundI].probes[probeI];
		probe->status = icmp->type == ICMP_TIME_EXCEEDED ? TIME_EXCEEDED :
			icmp->type == ICMP_DEST_UNREACH ? (icmp->code == ICMP_PORT_UNREACH ? PORT_UNREACHABLE :
			icmp->code == ICMP_NET_UNREACH ? NETWORK_UNREACHABLE :
			icmp->code == ICMP_HOST_UNREACH ? HOST_UNREACHABLE :
			icmp->code == ICMP_PROT_UNREACH ? PROTOCOL_UNREACHABLE : UNKNOWN_ERROR) : UNKNOWN_ERROR;
		getnameinfo((struct sockaddr *)&from, sizeof(from), probe->hopIpName,
			sizeof(probe->hopIpName), NULL, 0, NI_NUMERICHOST);
		gettimeofday(&context->rounds[roundI].probes[probeI].receiveTime, NULL);

		#ifdef DEBUG
			printf("Received ICMP packet: type=%d, code=%d, round=%u, probe=%u, status=%d, hopIpName=%s\n",
				icmp->type, icmp->code, roundI, probeI, probe->status, probe->hopIpName);
		#endif
	}
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

	#ifdef DEBUG
		printf("Sent UDP packet: ttl=%d, port=%d, round=%u, probe=%u\n",
			ttl, probe->port, ttl - 1, nextIndex % queries);
	#endif
}
