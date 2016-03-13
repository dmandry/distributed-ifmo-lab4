/**
 * @file     load.h
 * @Author   Andrey Dmitriev (dmandry92@gmail.com)
 * @date     September, 2015
 * @brief    Buisness-process functionality
 */

#include "load.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pa2345.h"
#include "process_msg.h"
#include "process_transmission.h"

extern int mutexl;
extern FILE *eventlog;
extern timestamp_t lamport_time;
extern int8_t num_processes;

char buff[100];
request_queue_t rq[11] = {{0,0,9999},{0,0,9999},{0,0,9999},{0,0,9999},{0,0,9999},
                          {0,0,9999},{0,0,9999},{0,0,9999},{0,0,9999},{0,0,9999},
                          {0,0,9999}};
int wait_flag = 0;
int8_t num_running_processes;
int8_t num_future_processes;
timestamp_t request_time;

int request_cs(const void * self)
{
	local_id i;
	local_id id = ((request_queue_t *)(self))->id;
	local_id req_id = ((request_queue_t *)(self))->req_id;
	timestamp_t req_time = ((request_queue_t *)(self))->req_time;
	for (i=num_processes-1; i>=0; i--)
	{
		if (rq[i].req_time > req_time ||
		    (rq[i].req_time == req_time && id <= rq[i].req_id))
		{
			rq[i+1] = rq[i];
		}
		else
		{
			rq[i+1] = *((request_queue_t *)(self));
			i = -2;
		}
	}
	if (i == -1)
	{
		rq[0] = *((request_queue_t *)(self));
	}
	if (id != req_id)
    {
	    process_send(id, req_id, CS_REPLY);
	}
	if (id != rq[0].req_id && rq[0].id != 0)
	{
		return 1;
	}
	else
	{
        return 0;
	}
}

int release_cs(const void * self)
{
	local_id i;
	for (i = 0; i<num_processes; i++)
	{
		rq[i] = rq[i+1];
	}
	return 0;
}

void load(local_id id)
{
	int i;
	request_queue_t *rqe = NULL;
	for (i=1; i<=id*5;i++)
	{
		sprintf(buff, log_loop_operation_fmt, id, i, id*5);
		if (mutexl)
		{
			rqe = malloc(sizeof(request_queue_t));
			rqe->id = id;
			rqe->req_id = id;
			process_send_multicast(id, CS_REQUEST);
			rqe->req_time = get_lamport_time();
			request_cs(rqe);
			wait_flag = 1;
			num_future_processes = 1;
			request_time = rqe->req_time;
		    while (num_future_processes != num_running_processes && wait_flag)
			{
			    process_recieve_any(id);
			}
		}
		print(buff);
		if (mutexl)
		{
			process_send_multicast(id, CS_RELEASE);
			release_cs(rqe);
			free(rqe);
		}
	}
}
