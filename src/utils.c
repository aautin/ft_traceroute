#include "ft_traceroute.h"

#include <sys/time.h>

struct timeval timeval_mul(struct timeval tv, double factor)
{
	int64_t us = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
	us = (int64_t)(us * factor);

	struct timeval result;
	result.tv_sec = us / 1000000;
	result.tv_usec = us % 1000000;

	if (result.tv_usec < 0)
	{
		result.tv_sec--;
		result.tv_usec += 1000000;
	}

	return result;
}

uint64_t getWaitingForReplyNumber(t_rounds *rounds, t_options options)
{
	uint64_t waitingForReply = 0;
	for (uint8_t i = 0; i < options.maxHops; ++i)
	{
		for (uint8_t j = 0; j < options.queries; ++j)
		{
			if (rounds[i].probes[j].status == WAITING_FOR_REPLY)
			{
				waitingForReply++;
			}
		}
	}
	return waitingForReply;
}

struct timeval getTimeout(const t_options options, t_rounds *rounds, uint8_t roundI)
{
	struct timeval hereRtt;
	struct timeval nearRtt;

	for (uint8_t j = 0; j < options.queries; ++j)
	{
		t_probe* probe = &rounds[roundI].probes[j];
		if (probe->status != WAITING_FOR_REPLY && probe->status != WAITING_TO_SEND)
		{
			struct timeval rtt;
			timersub(&probe->receiveTime, &probe->sendTime, &rtt);
			if (!timerisset(&hereRtt))
			{
				hereRtt = rtt;
			}
			else if (timercmp(&rtt, &hereRtt, >))
			{
				hereRtt = rtt;
			}
		}
	}

	if (roundI < options.maxHops - 1)
	{
		for (uint8_t j = 0; j < options.queries; ++j)
		{
			t_probe* probe = &rounds[roundI + 1].probes[j];
			if (probe->status != WAITING_FOR_REPLY && probe->status != WAITING_TO_SEND)
			{
				struct timeval rtt;
				timersub(&probe->receiveTime, &probe->sendTime, &rtt);
				if (!timerisset(&nearRtt))
				{
					nearRtt = rtt;
				}
				else if (timercmp(&rtt, &nearRtt, >))
				{
					nearRtt = rtt;
				}
			}
		}
	}

	struct timeval timeout;

	if (timerisset(&hereRtt))
	{
		timeout = timeval_mul(hereRtt, options.wait.here);
	}
	else
	{
		timeout = options.wait.max;
	}

	if (timerisset(&nearRtt))
	{
		struct timeval nearTimeout = timeval_mul(nearRtt, options.wait.near);
		if (timercmp(&nearTimeout, &timeout, >))
		{
			timeout = nearTimeout;
		}
	}

	#ifdef DEBUG
		#include <stdio.h>
		printf("Timeout for round %d: %ld.%06ld seconds\n", roundI + 1, timeout.tv_sec, timeout.tv_usec);
	#endif

	return timeout;
}
