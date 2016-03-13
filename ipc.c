/**
 * @file     ipc.c
 * @Author   Andrey Dmitriev (dmandry92@gmail.com)
 * @date     September, 2015
 * @brief    A simple IPC implementation for programming assignments
 */

#include "ipc.h"
#include <unistd.h>

local_id recieve_any_dst = -1;

int send(void * self, local_id dst, const Message * msg)
{
	if (msg == NULL || self == NULL)
		return -1;
	if (write(((int *)self)[dst],
			  msg,
			  sizeof(MessageHeader) +
			  (msg->s_header.s_payload_len)) == -1)
		return -1;
	return 0;
}

int send_multicast(void * self, const Message * msg)
{
	local_id i;
	if (msg == NULL || self == NULL)
		return -1;
	i = 0;
	while (((int *)self)[i] != -2)
	{
		if (((int *)self)[i] != -1)
            if (send(self, i, msg) != 0)
			    return -1;
		i++;
	}
	return 0;
}

int receive(void * self, local_id from, Message * msg)
{
	MessageHeader msg_hdr;
	if (msg == NULL || self == NULL)
		return -1;
    if ((read(((int *)self)[from],
			  &msg_hdr,
			  sizeof(MessageHeader))) == sizeof(MessageHeader))
    {
		msg->s_header = msg_hdr;
		if (msg_hdr.s_payload_len == 0 ||
		    read(((int *)self)[from],
				 msg->s_payload,
				 msg_hdr.s_payload_len) == msg_hdr.s_payload_len)
			return 0;
		else
			return -1;
	}
	return 1;
}

int receive_any(void * self, Message * msg)
{
	local_id i;
	int res;
	if (msg == NULL || self == NULL)
		return -1;
	i = 0;
	while (((int *)self)[i] != -2)
	{
		if (((int *)self)[i] != -1)
		{
			recieve_any_dst = i;
			res = receive(self, i, msg);
		    if (res != 1)
			    return res;
		}
		i++;
	}
	return 1;
}
