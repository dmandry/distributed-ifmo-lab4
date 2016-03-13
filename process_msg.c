/**
 * @file     process_msg.c
 * @Author   Andrey Dmitriev (dmandry92@gmail.com)
 * @date     September, 2015
 * @brief    Message processing implementation
 */

#include "process_msg.h"

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "pa2345.h"
#include "load.h"

extern FILE *eventlog;
extern int8_t num_running_processes;
extern int8_t num_processes;
extern int8_t num_future_processes;
extern timestamp_t request_time;
extern int wait_flag;

int started[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int done[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int reply[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int8_t started_num = 0;
int8_t done_num = 0;
int8_t reply_num = 0;

int started_len = 0;
int done_len = 0;

timestamp_t lamport_time = 0;

timestamp_t get_lamport_time()
{
	return lamport_time;
}

void process_msg_started(Message *msg)
{
	int id, pid, ppid, time, bal;
	msg->s_payload[msg->s_header.s_payload_len] = '\0';
	sscanf(msg->s_payload, log_started_fmt, &time, &id, &pid, &ppid, &bal);
	if (started[id] == 0) started_num++;
	started[id]++;
}

void process_msg_done(Message *msg)
{
	int id, time, bal;
	msg->s_payload[msg->s_header.s_payload_len] = '\0';
	sscanf(msg->s_payload, log_done_fmt, &time, &id, &bal);
	if (done[id] == 0)
	{
		done_num++;
	}
	num_running_processes--;
	done[id]++;
}

void process_msg(Message *msg, local_id id, local_id dst)
{
	request_queue_t *rqe = malloc(sizeof(request_queue_t));
	lamport_time++;
	while (lamport_time <= msg->s_header.s_local_time)
    {
		lamport_time++;
    }
	switch (msg->s_header.s_type)
	{
		case STARTED:
		    process_msg_started(msg);
		    break;
		case DONE:
		    process_msg_done(msg);
		    break;
		case CS_REQUEST:
			if (id == PARENT_ID || done[id] > 0) break;
			rqe->id = id;
			rqe->req_id = dst;
			rqe->req_time = msg->s_header.s_local_time;
			if (request_time < msg->s_header.s_local_time ||
			    (request_time == msg->s_header.s_local_time && id < dst))
		    {
				num_future_processes++;
			}
			wait_flag = request_cs(rqe);
			break;
		case CS_REPLY:
			break;
		case CS_RELEASE:
			if (id == PARENT_ID || done[id] > 0) break;
			rqe->id = id;
			rqe->req_id = dst;
			rqe->req_time = msg->s_header.s_local_time;
            release_cs(rqe);
			break;
		default:
			fclose(eventlog);
			free(msg);
			free(rqe);
		    perror("Unknown msg type");
		    exit(EXIT_FAILURE);
	}
	free(rqe);
}

int payload_size(int16_t type)
{
	switch (type)
	{
		case STARTED:
		    return started_len;
		    break;
		case DONE:
		    return done_len;
		    break;
		case CS_RELEASE:
		case CS_REPLY:
		case CS_REQUEST:
			return 0;
            break;
		default:
			return MAX_PAYLOAD_LEN;
	}
	fclose(eventlog);
	perror("WTF");
	exit(EXIT_FAILURE);
}

Message *create_msg(int16_t type, char *payload)
{
	uint16_t i;
	Message *msg;
	uint16_t payload_len;
	payload_len = payload_size(type);
	if (payload == NULL && payload_len != 0)
	{
		fclose(eventlog);
		perror("create_msg");
		exit(EXIT_FAILURE);
	}
	msg = malloc(sizeof(MessageHeader)+payload_len);
	if (msg == NULL)
	{
		fclose(eventlog);
		if (payload_len != 0)
		{
		    free(payload);
		}
		perror("create_msg");
		exit(EXIT_FAILURE);
	}
	msg->s_header.s_magic = MESSAGE_MAGIC;
	msg->s_header.s_type = type;
	msg->s_header.s_payload_len = payload_len;
	if (type >= 0)
	{
		lamport_time++;
	}
	msg->s_header.s_local_time = get_lamport_time();
	for (i=0; i<payload_len; i++)
	    msg->s_payload[i] = payload[i];
	return msg;
}

const char *log_fmt_type(int16_t type)
{
	switch (type)
	{
		case STARTED:
		    return log_started_fmt;
		    break;
		case DONE:
		    return log_done_fmt;
		    break;
		default:
			fclose(eventlog);
		    perror("Unknown msg type");
		    exit(EXIT_FAILURE);
	}
	fclose(eventlog);
	perror("WTF");
	exit(EXIT_FAILURE);
}

void create_typed_payload(int16_t type,
                          local_id id,
                          char *payload)
{
	switch (type)
	{
		case STARTED:
	        sprintf(payload,
	                log_started_fmt,
	                get_lamport_time()+1,
	                id,
	                getpid(),
	                getppid(),
	                0);
			break;
		case DONE:
	        sprintf(payload,
	                log_done_fmt,
	                get_lamport_time()+1,
	                id,
	                0);
			break;
	}
}

char *create_payload(int16_t type, local_id id)
{
	int payload_len = payload_size(type);
	char *payload = NULL;
	if (payload_len != 0)
	{
	    payload = malloc(payload_len);
	}
	if (payload == NULL && payload_len != 0)
	{
	    fclose(eventlog);
	    perror("create_payload");
	    exit(EXIT_FAILURE);
	}
	create_typed_payload(type, id, payload);
	return payload;
}

void count_sent_num(local_id id, int16_t type)
{
	switch (type)
	{
		case STARTED:
		    started[id]++;
			started_num++;
		    break;
		case DONE:
			done[id]++;
			done_num++;
		    break;
	}
}

int8_t *get_rcvd_num(int16_t type)
{
	switch (type)
	{
		case STARTED:
		    return &started_num;
		    break;
		case DONE:
		    return &done_num;
		    break;
		default:
			fclose(eventlog);
		    perror("Unknown msg type");
		    exit(EXIT_FAILURE);
	}
	fclose(eventlog);
	perror("WTF");
	exit(EXIT_FAILURE);
}

int *get_rcvd(int16_t type)
{
	switch (type)
	{
		case STARTED:
		    return started;
		    break;
		case DONE:
		    return done;
		    break;
		default:
			fclose(eventlog);
		    perror("Unknown msg type");
		    exit(EXIT_FAILURE);
	}
	perror("WTF");
	exit(EXIT_FAILURE);
}
