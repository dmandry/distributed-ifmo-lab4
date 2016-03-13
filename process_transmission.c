/**
 * @file     process_transmission.c
 * @Author   Andrey Dmitriev (dmandry92@gmail.com)
 * @date     September, 2015
 * @brief    Transmission organisation implementation 
 */

#include "process_transmission.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "load.h"
#include "process_msg.h"

extern FILE *eventlog;
extern local_id recieve_any_dst;

char free_payload[MAX_PAYLOAD_LEN];
int pipefds_to_write[12][12];
int pipefds_to_read[12][12];

void create_pipe_topology(int8_t num_processes, FILE *pipelog)
{
	local_id i,j;
	for (i=0; i<num_processes+1; i++)
	{
		for (j=0; j<num_processes+1; j++)
	    {
	        int pipefd[2];
			if (i == j)
			{
				pipefds_to_read[j][i] = -1;
				pipefds_to_write[i][j] = -1;
				continue;
			}
		    if (pipe2(pipefd, O_NONBLOCK) == -1)
		    {
				fclose(pipelog);
				fclose(eventlog);
			    perror("pipe");
			    exit(EXIT_FAILURE);
		    }
			fprintf(pipelog, "%i %i\n", pipefd[0], pipefd[1]);
		    pipefds_to_read[j][i] = pipefd[0];
			pipefds_to_write[i][j] = pipefd[1];
	    }
		pipefds_to_read[i][num_processes+1] = -2;
		pipefds_to_write[i][num_processes+1] = -2;
		pipefds_to_read[num_processes+1][i] = -2;
		pipefds_to_write[num_processes+1][i] = -2;
	}
	fclose(pipelog);
}

void close_unused_pipes(int8_t num_processes, local_id id)
{
	local_id i,j;
	for (i=0; i<num_processes+1; i++)
		for (j=0; (i!=id) && (j<num_processes+1); j++)
	    {
	        if (pipefds_to_read[i][j] != -1)
	        {
			    if (close(pipefds_to_read[i][j]) != 0)
				{
					fclose(eventlog);
					perror("close_unused_pipes");
					exit(EXIT_FAILURE);
				}
				pipefds_to_read[i][j] = -1;
		    }
			if (pipefds_to_write[i][j] != -1)
	        {
			    if (close(pipefds_to_write[i][j]) != 0)
				{
					fclose(eventlog);
					perror("close_unused_pipes");
					exit(EXIT_FAILURE);
				}
				pipefds_to_write[i][j] = -1;
		    }
		}
}

void close_used_pipes(int8_t num_processes, local_id id)
{
	local_id j;
    for (j=0; j<num_processes+1; j++)
	{
	    if (pipefds_to_read[id][j] != -1)
	    {
			if (close(pipefds_to_read[id][j]) != 0)
		    {
				fclose(eventlog);
				perror("close_unused_pipes");
				exit(EXIT_FAILURE);
			}
			pipefds_to_read[id][j] = -1;
		}
		if (pipefds_to_write[id][j] != -1)
	    {
			if (close(pipefds_to_write[id][j]) != 0)
			{
				fclose(eventlog);
				perror("close_unused_pipes");
				exit(EXIT_FAILURE);
			}
			pipefds_to_write[id][j] = -1;
		}
	}
}

void process_send_multicast(local_id id, int16_t type)
{
	char *payload;
	Message *msg;
	payload = create_payload(type, id);
    msg = create_msg(type, payload);
	free(payload);
	if (send_multicast(pipefds_to_write[id], msg) != 0)
	{
		fclose(eventlog);
		free(msg);
		perror("send_multicast");
		exit(EXIT_FAILURE);
	}
    count_sent_num(id, type);
	free(msg);
	msg = NULL;
}

void process_send(local_id from,
                  local_id to,
                  int16_t type)
{
	char *payload;
	Message *msg;
	payload = create_payload(type, from);
    msg = create_msg(type, payload);
	free(payload);
	if (send(pipefds_to_write[from], to, msg) != 0)
	{
		fclose(eventlog);
		free(msg);
		perror("send");
		exit(EXIT_FAILURE);
	}
    count_sent_num(from, type);
	free(msg);
	msg = NULL;
}

void process_recieve_all(int8_t num_processes, local_id id, int16_t type)
{
	Message *msg = NULL;
	local_id i;
	int res;
	int8_t *rcvd_num;
	int *rcvd;
	int watchdog;
	watchdog = 0;
	rcvd_num = get_rcvd_num(type);
	rcvd = get_rcvd(type);
	while(*rcvd_num < num_processes && watchdog < 100000000)
	{
		for (i=1;i<num_processes+1;i++)
		{
			msg = create_msg(-1, free_payload);
			if (rcvd[i] == 0)
			{
		        res = receive(pipefds_to_read[id], i, msg);
		        if (res == 0)
	                process_msg(msg, id, i);
		        else
			        if (res == -1)
		            {
				        fclose(eventlog);
				        free(msg);
			            perror("receive_any");
			            exit(EXIT_FAILURE);
		            }
			}
			free(msg);
		    msg = NULL;
	    }
		watchdog++;
	}
}

void process_recieve_any(local_id id)
{
	Message *msg = NULL;
	msg = create_msg(-1, free_payload);
	int res;
	res = receive_any(pipefds_to_read[id], msg);
	if (res == -1)
	{
        fclose(eventlog);
	    free(msg);
		perror("receive_any");
		exit(EXIT_FAILURE);
	}
	if (res == 0)
	{
		process_msg(msg, id, recieve_any_dst);
	}
	free(msg);
	msg = NULL;
}

void process_load(local_id id)
{
	load(id);
}
