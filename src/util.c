#include "util.h"

void exit_with_error(const char *msg, struct self_data *my_data)
{
	// Print the error message in red
	fprintf(stderr, "\t\033[1;31m%s\033[0m\n", msg);
	close_all_sockets(my_data);

	// Exit the program with an error code
	exit(1);
}

void print_state(int state_number) // prints the state to easily see where program is
{
	printf("\033[32m[Q%d]", state_number);
	switch (state_number)
	{
	case 12:
		printf("[NET_JOIN]");
		break;
	case 15:
		printf("[NET_NEW_RANGE]");
		break;
	case 16:
		printf("[NET_LEAVING]");
		break;
	case 17:
		printf("[NET_CLOSE_CONNECTION]");
		break;
	case 5:
		printf("[Not connected]");
		break;
	case 13:
		printf("[I have the largest span]");
		break;
	case 14:
		printf("[Forwarding net_join]");
		break;
	default:
		break;
	}

	printf("\033[0m\n");
}

void close_all_sockets(struct self_data *self_data) // close all sockets
{
	close(self_data->listening.socket);
	close(self_data->udp_socket);
	close(self_data->successor.socket);
	close(self_data->predecessor.socket);
}