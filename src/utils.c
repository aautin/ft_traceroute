#include "ft_traceroute.h"

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
