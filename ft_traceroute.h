#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>
#include <netinet/in.h>


typedef struct s_wait
{
	struct timeval max;
	uint8_t        here;
	uint8_t        near;
} t_wait;

typedef struct s_options
{
	uint8_t	 simQueries;
	uint8_t	 maxHops;
	uint16_t port;
	uint8_t	 queries;
	t_wait   wait;
} t_options;

typedef struct s_host
{
	char            *name;
	struct sockaddr  address;
	char             ipName[INET_ADDRSTRLEN];
} t_host;

enum e_status
{
	WAITING_TO_SEND,
	WAITING_FOR_REPLY,

	// TTL expired
	TIME_EXCEEDED,
	
	// Error from an intermediary router
	NETWORK_UNREACHABLE,
	HOST_UNREACHABLE,
	PROTOCOL_UNREACHABLE,
	UNKNOWN_ERROR,

	// No timely response
	TIMEOUT,

	// Reply from the destination host
	PORT_UNREACHABLE,
};

typedef struct s_probe
{
	uint16_t       port;
	struct timeval sendTime;
	struct timeval receiveTime;

	enum e_status status;
	char          statusIp[INET_ADDRSTRLEN];
} t_probe;

typedef struct s_rounds
{
	t_probe* probes;
	uint8_t  ttl;
} t_rounds;

typedef struct s_context
{
	t_options options;
	t_host    host;
	
	int icmpSocket;
	int udpSocket;

	t_rounds *rounds;
} t_context;

// -------------- Functions -------------- //

// Init
int  initOptions(t_options *options, int argc, char **argv);
void initHost(t_host *host, int nextIndex, int argc, char **argv);
void initSockets(t_context *context);
void initProbes(t_rounds **rounds, t_options *options);

// Error
void missingHostError();
void badOptionError(char *option, int index);
void argumentRequiredError(char *option, int index);
void badArgument(char *option, char *argument, int index);
void extraArgumentError(char *argument, int index);
void fatalError(char *message);
void argumentTooBigError(char *option, long limit);

// Output
bool isOutputUpdated(const t_rounds *rounds, const t_options options);
void announceHelp();
void announceOptions(t_options *options);
void updateOutput(const t_context *context);

// Utils
uint64_t getWaitingForReplyNumber(t_rounds *rounds, t_options options);

// Packet
void readReplies(t_context *context);
void sendNextProbe(t_context *context, uint32_t nextProbeIndex);
