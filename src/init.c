#include "ft_traceroute.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <string.h>

int initOptions(t_options *options, int argc, char **argv)
{
	const char          shortOpts[]   = ":?N:m:p:q:w:";
	const struct option longOptions[] = {
		{"help",        no_argument,       NULL, '?'},
		{"sim-queries", required_argument, NULL, 'N'},
		{"max-hops",    required_argument, NULL, 'm'},
		{"port",        required_argument, NULL, 'p'},
		{"queries",     required_argument, NULL, 'q'},
		{"wait",        required_argument, NULL, 'w'},
		{}
	};

	*options = (t_options) {
		.simQueries = 16,
		.maxHops    = 30,
		.port       = 33434,
		.queries    = 3,
		.wait       = {
			.max  = { .tv_sec = 5, .tv_usec = 0 },
			.here = 3,
			.near = 10
		}
	};

	while (true)
	{
		int opt = getopt_long(argc, argv, shortOpts, longOptions, NULL);
		if (opt == -1)
		{
			break;
		}
		
		switch (opt)
		{
			case ':':
			{
				argumentRequiredError(argv[optind - 1], optind - 1);
				break;
			}
			case '?':
			{
				if (strcmp(argv[optind - 1], "--help") && strcmp(argv[optind - 1], "-?"))
				{
					badOptionError(argv[optind - 1], optind - 1);
				}
				else
				{
					announceHelp();
					exit(EXIT_SUCCESS);
				}
			}
			case 'N':
			{
				char* endPtr;
				long  value = strtol(optarg, &endPtr, 10);
				if (*endPtr != '\0')
				{
					badArgument(argv[optind - 2], optarg, optind - 1);
				}

				options->simQueries = (uint8_t)value;
				break;
			}
			case 'm':
			{
				char*    endPtr;
				uint64_t value = strtol(optarg, &endPtr, 10);
				if (*endPtr != '\0')
				{
					badArgument(argv[optind - 2], optarg, optind - 1);
				}
				if (value > 255)
				{
					argumentTooBigError(argv[optind - 2], 255);
				}

				options->maxHops = (uint8_t)value;
				break;
			}
			case 'p':
			{
				char*    endPtr;	
				uint64_t value = strtol(optarg, &endPtr, 10);
				if (*endPtr != '\0')
				{
					badArgument(argv[optind - 2], optarg, optind - 1);
				}
				options->port = (uint16_t)value;
				break;
			}
			case 'q':
			{
				char*    endPtr;
				uint64_t value = strtol(optarg, &endPtr, 10);
				if (*endPtr != '\0')
				{
					badArgument(argv[optind - 2], optarg, optind - 1);
				}
				if (value > 10)
				{
					argumentTooBigError(argv[optind - 2], 10);
				}
	
				options->queries = (uint8_t)value;
				break;
			}
			case 'w':
			{
				char* maxPtr;
				double max = strtod(optarg, &maxPtr);
				options->wait.max.tv_sec  = (time_t)max;
				options->wait.max.tv_usec = (suseconds_t)((max - options->wait.max.tv_sec) * 1e6);
				if (*maxPtr != '\0')
				{
					if (*maxPtr == ',')
					{
						char* herePtr;
						options->wait.here = (uint8_t)strtol(++maxPtr, &herePtr, 10);
						if (*herePtr != '\0')
						{
							if (*herePtr == ',')
							{
								char* nearPtr;
								options->wait.near = (uint8_t)strtol(++herePtr, &nearPtr, 10);
								if (*nearPtr != '\0')
								{
									badArgument(argv[optind - 2], optarg, optind - 1);
								}
							}
							else
							{
								badArgument(argv[optind - 2], optarg, optind - 1);
							}
						}
					}
					else
					{
						badArgument(argv[optind - 2], optarg, optind - 1);
					}
				}
				break;
			}
		}
	}
	
	#ifdef DEBUG
		announceOptions(options);
	#endif

	return optind;
}

void initHost(t_host *host, int nextIndex, int argc, char **argv)
{
	if (nextIndex >= argc)
	{
		missingHostError();
	}

	if (nextIndex < argc - 1)
	{
		extraArgumentError(argv[nextIndex + 1], nextIndex + 1);
	}

	host->name = argv[nextIndex];

	//
	// DNS resolution
	//
	struct addrinfo *info;
	struct addrinfo  hints = {
	    .ai_family    = AF_INET,
	    .ai_socktype  = SOCK_RAW,
		.ai_flags     = 0,
	    .ai_protocol  = IPPROTO_ICMP,
	    .ai_canonname = NULL,
	    .ai_addr      = NULL,
	    .ai_next      = NULL,
	};
	int status = getaddrinfo(host->name, NULL, &hints, &info); 
	if (status)
	{
		fatalError("unknown host");
	}

	//
	// Store the resolved IP address and convert it to a string representation
	//
	host->address = *info->ai_addr;
	inet_ntop(AF_INET, &((struct sockaddr_in*)info->ai_addr)->sin_addr,
		host->ipName, sizeof(host->ipName));
	
	#ifdef DEBUG
		#include <stdio.h>
		printf("Resolved host: %s -> %s\n", host->name, host->ipName);
	#endif

	freeaddrinfo(info);
}

void initSockets(t_context *context)
{
	//
	// We know only 3 types of sockets: STREAM, DGRAM, and RAW.
	// Network-layer (3) are IP, ICMP, and ARP (RAW)
	// Transport-layer (4) are TCP and UDP (STREAM and DGRAM)
	// 

	//
	// STREAM : You are at the transport layer, with a stream abstraction
	// 
	// I used it for an IRC-server project : https://github.com/aautin/42_irc
	//
	// Packets are sent and received in a continuous stream of bytes
	// A message can be split into several packets, must be reassembled
	//
	// The kernel handles IP headers, TCP segments and packet boundaries
	// The destination is define by a connection (connect() or accept())
	//
	// We only see the stream of bytes : underlying network visibility is LOW
	//

	//
	// DGRAM : You are at the transport layer, with message boundaries preserved
	// 
	// Never used it, but here I will for sending UDP packets
	//
	// Packets are sent and received as discrete messages (1 packet = 1 message)
	// 
	// The kernel handles IP headers and UDP datagrams
	// The destination is defined at send or receive time (sendto() or recvfrom())
	//
	// We only see the discrete messages : underlying network visibility is LOW
	//

	//
	// RAW : You are at the network layer
	//
	// I used it for a ping project : https://github.com/aautin/ft_ping
	//
	// Packets are following the IP protocol, with headers and payloads
	//
	// IP header can be accessible (if setockopt() is used with IP_HDRINCL)
	//
	// When receiving a packet : underlying network visibility is HIGH
	// we can see the IP header and its payload
	//

	//
	// DGRAM UDP socket is used here to send the UDP packets (with incrementing TTL)
	//
	context->udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (context->udpSocket < 0)
	{
		fatalError("udp socket()");
		exit(EXIT_FAILURE);
	}

	//
	// RAW ICMP socket is used here to receive ICMP messages (the replies)
	// From the intermediary routers : [T11 C0]  Time Exceeded (TTL = 0) -> Time of it
	//                                 [T3  C0]  Network unreachable     -> !N
	//                                 [T3  C1]  Host unreachable		 -> !H
	//								   [T3  C2]  Protocol unreachable    -> !P
	//
	// From	the destination host :     [T3  C3]  Port unreachable
	//
	context->icmpSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (context->icmpSocket < 0)
	{
		fatalError("icmp socket()");
		exit(EXIT_FAILURE);
	}

	//
	// Set a timeout of 0.1 ms for the ICMP socket to avoid blocking recvfrom
	//
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100;
	setsockopt(context->icmpSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

void initProbes(t_rounds **rounds, t_options *options)
{
	*rounds = malloc(sizeof(t_rounds) * options->maxHops);
	if (!*rounds)
	{
		fatalError("malloc()");
	}

	for (uint8_t i = 0; i < options->maxHops; ++i)
	{
		(*rounds)[i].probes = malloc(sizeof(t_probe) * options->queries);
		if (!(*rounds)[i].probes)
		{
			fatalError("malloc()");
		}

		for (uint8_t j = 0; j < options->queries; ++j)
		{
			(*rounds)[i].probes[j].status = WAITING_TO_SEND;
		}
	}
}