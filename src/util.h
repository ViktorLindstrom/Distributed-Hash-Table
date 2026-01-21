#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <netinet/in.h>
#include <poll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hashtable.h"

#define MAX_SIZE 256
struct connection_point
{
	int socket;
	struct sockaddr_in dest_addr;
};
struct self_data
{
	struct ht *hash_table;
	uint8_t range_start;
	uint8_t range_end;

	struct pollfd fds[4];

	bool alive;
	struct in_addr my_ip_addr;
	struct sockaddr_in tracker_addr;

	int udp_socket;
	struct connection_point successor;
	struct connection_point predecessor;
	struct connection_point listening;
	char *ssns[MAX_SIZE];
};

void exit_with_error(const char *msg, struct self_data *my_data);

void print_state(int state_number);
void close_all_sockets(struct self_data *self_data);
#endif