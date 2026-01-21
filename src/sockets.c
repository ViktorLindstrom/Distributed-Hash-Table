
#include "sockets.h"

int send_udp_pdu(int socket, struct sockaddr_in dest, const void *pdu, size_t pdu_size)
{

	ssize_t bytes_sent = sendto(socket, pdu, pdu_size, 0,
				    (struct sockaddr *)&dest, sizeof(dest));
	if (bytes_sent < 0)
	{
		perror("Failed to send PDU");
		return -1;
	}

	// printf("\tSent %ld bytes to %s:%d\n", bytes_sent, inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));

	return 0;
}

int send_tcp_pdu(int sockfd, const void *pdu, size_t pdu_size)
{
	struct pollfd fd;
	fd.fd = sockfd;
	fd.events = POLLOUT;

	int ret = poll(&fd, 1, 5000); // poll
	if (ret <= 0)
	{
		if (ret == 0)
			fprintf(stderr, "Socket write timeout\n");
		else
			perror("Poll failed");
		return -1;
	}

	if (!(fd.revents & POLLOUT))
	{
		fprintf(stderr, "Socket not ready for writing\n");
		return -1;
	}

	ssize_t bytes_sent = send(sockfd, pdu, pdu_size, 0);
	if (bytes_sent < 0)
	{
		if (errno == EPIPE)
		{
			fprintf(stderr, "Connection broken (EPIPE)\n");
		}
		else
		{
			perror("Failed to send PDU");
		}
		return -1; // Error in sending PDU
	}

	if (bytes_sent != pdu_size)
	{
		fprintf(stderr, "Warning: Not all bytes were sent. Sent %ld out of %zu bytes\n", bytes_sent, pdu_size);
	}
	return 0;
}

int receive_from(struct pollfd *fds, int fd, int sockfd, void *pdu, size_t pdu_size)
{
	int poll_response = poll(fds, 1, 10000);
	if (poll_response <= 0)
	{
		perror("poll timeout or fail");
		exit(1);
	}
	if (fds[fd].revents & POLLIN)
	{
		struct sockaddr_in src_addr;
		socklen_t addr_len = sizeof(src_addr);
		ssize_t bytes_received = recvfrom(sockfd, pdu, pdu_size, 0, (struct sockaddr *)&src_addr, &addr_len);
		if (bytes_received < 0)
		{
			printf("Failed to receive PDU\n");
			return -1;
		}
		if (bytes_received < pdu_size)
		{
			fprintf(stderr, "Warning: Received fewer bytes than expected. Received %ld out of %zu bytes\n", bytes_received, pdu_size);
		}
		return 0;
	}
	else
	{
		return -1;
	}
}

void receive_tcp_data(struct self_data *my_data, char *node_to_receive, char *buffer, size_t buffer_size)
{

	int socket_fd = 0; // Choose the correct socket based on the node to receive data from
	if (strcmp(node_to_receive, "predecessor") == 0)
	{
		socket_fd = my_data->predecessor.socket;
	}
	else if (strcmp(node_to_receive, "successor") == 0)
	{
		socket_fd = my_data->successor.socket;
	}
	else
	{
		exit_with_error("Incorrect node specified for receiving data", my_data);
	}

	// Set up pollfd structure to monitor the socket for readability
	struct pollfd fds[1];
	fds[0].fd = socket_fd;
	fds[0].events = POLLIN; // We want to check for incoming data

	int timeout = 5000;

	if (poll(fds, 1, timeout) <= 0) // Poll the socket to see if data is available to read
		exit_with_error("Failed to received data", my_data);

	// If the socket is ready for reading, read the data
	if (fds[0].revents & POLLIN)
	{
		ssize_t bytes_received = recv(socket_fd, buffer, buffer_size, 0);

		if (bytes_received < 0)
		{
			perror("Receive failed");
			exit_with_error("Failed to receive data", my_data);
		}
		else if (bytes_received == 0)
		{
			// Connection closed by the peer
			printf("Connection closed by %s\n", node_to_receive);
			return;
		}
	}
}

struct sockaddr_in create_destination_addr(const char *ip, uint16_t port, struct self_data *self_data)
{
	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &dest.sin_addr) <= 0)
	{
		exit_with_error("Failed to convert IP address", self_data);
	}
	return dest;
}

int create_udp_sock(struct self_data *self_data)
{
	int socket;
	struct sockaddr_in addr;
	// Create socket
	socket = create_socket(AF_INET, SOCK_DGRAM, 0, self_data);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(0);

	if (bind(socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		exit_with_error("Failed to bind socket", self_data);
	}

	// Use getsockname to retrieve the assigned port
	socklen_t addr_len = sizeof(addr);
	if (getsockname(socket, (struct sockaddr *)&addr, &addr_len) < 0)
	{
		exit_with_error("Failed to retrieve socket name", self_data);
	}

	if (set_nonblocking(socket, self_data) < 0)
		exit_with_error("Failed to set socket to non-blocking", self_data);

	printf("\tListening on UDP on port %d\n", ntohs(addr.sin_port));
	return socket;
}
int create_listening_tcp_sock(struct self_data *self_data)
{
	int socket;
	struct sockaddr_in addr;

	socket = create_socket(AF_INET, SOCK_STREAM, 0, self_data);

	if (set_nonblocking(socket, self_data) < 0)
		exit_with_error("Failed to set socket to non-blocking", self_data);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(0);

	if (bind(socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		exit_with_error("Failed to bind socket", self_data);
	}

	socklen_t addr_len = sizeof(addr); // Use getsockname to retrieve the assigned port
	if (getsockname(socket, (struct sockaddr *)&addr, &addr_len) < 0)
	{
		exit_with_error("Failed to retrieve socket name", self_data);
	}

	printf("\tListening on TCP port %d\n", ntohs(addr.sin_port));
	if (listen(socket, 1) < 0)
	{
		exit_with_error("Failed to listen on socket", self_data);
	}
	self_data->listening.dest_addr.sin_port = addr.sin_port;
	self_data->listening.dest_addr.sin_addr = self_data->my_ip_addr;
	self_data->listening.dest_addr.sin_family = AF_INET;

	return socket;
}

void accept_predecessor_connection(struct self_data *my_data)
{
	socklen_t addr_len = sizeof(my_data->predecessor.dest_addr);
	int client_socket = -1;

	struct pollfd *pfd = &my_data->fds[LISTENING_FDS];
	pfd->events = POLLIN;

	// Poll the listening socket for incoming connections
	int poll_result = poll(pfd, 1, -1); // Blocking indefinitely
	if (poll_result < 0)
	{
		perror("Poll failed");
		exit_with_error("Failed to poll listening socket", my_data);
	}
	else if (poll_result == 0)
	{
		printf("No events occurred, retrying...\n");
		return; // No events happened, return and retry later
	}

	// If poll indicates readiness (POLLIN event)
	if (pfd->revents & POLLIN)
	{
		// Accept the connection for the predecessor
		client_socket = accept(my_data->listening.socket,
				       (struct sockaddr *)&my_data->predecessor.dest_addr,
				       &addr_len);

		// Check if accept() succeeded
		if (client_socket < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				printf("No attempted connection from supposed predecessor\n");
				return;
			}
			else
			{
				perror("Accept failed");
				exit_with_error("Failed to accept connection", my_data);
			}
		}
		printf("\tAccepted TCP connection from predecessor\n");
		my_data->predecessor.socket = client_socket; // Accept worked! Assign the accepted socket to the predecessor field

		my_data->fds[PREDECESSOR_FDS].fd = client_socket; // Update the poll file descriptor

		my_data->fds[PREDECESSOR_FDS].events = POLLIN;
	}
}

void connect_to_tcp(struct self_data *self_data)
{
	self_data->successor.socket = create_socket(AF_INET, SOCK_STREAM, 0, self_data);
	if (self_data->successor.socket < 0)
		exit_with_error("Failed to create socket", self_data);

	if (set_nonblocking(self_data->successor.socket, self_data) < 0)
		exit_with_error("Failed to set socket to non-blocking", self_data);

	self_data->fds[SUCCESSOR_FDS].fd = self_data->successor.socket;
	self_data->fds[SUCCESSOR_FDS].events = POLLOUT;

	int ret = connect(self_data->successor.socket, (struct sockaddr *)&self_data->successor.dest_addr, sizeof(self_data->successor.dest_addr)); // set up connection
	if (ret < 0 && errno != EINPROGRESS)
	{
		perror("Connect failed immediately");
		close(self_data->successor.socket);
		self_data->successor.socket = -1;
		exit_with_error("Failed to connect to successor", self_data);
	}

	// Poll the socket for connection completion
	int ret2 = poll(self_data->fds, 4, 5000);
	if (ret2 > 0 && (self_data->fds[SUCCESSOR_FDS].revents & POLLOUT))
	{
		int error = 0;
		socklen_t len = sizeof(error);
		if (getsockopt(self_data->successor.socket, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0)
		{
			fprintf(stderr, "Connect failed: %s\n", strerror(error));
			close(self_data->successor.socket);
			self_data->successor.socket = -1;
			exit_with_error("Connection failed", self_data);
		}
	}
	else if (ret2 == 0)
	{
		fprintf(stderr, "Connection timed out.\n");
		close(self_data->successor.socket);
		self_data->successor.socket = -1;
		exit_with_error("Connection timed out", self_data);
	}
	else
	{
		perror("Poll failed");
		close(self_data->successor.socket);
		self_data->successor.socket = -1;
		exit_with_error("Poll failed", self_data);
	}

	printf("\tSuccessfully connected successor at %s:%u\n", inet_ntoa(self_data->successor.dest_addr.sin_addr), ntohs(self_data->successor.dest_addr.sin_port));

	self_data->fds[SUCCESSOR_FDS].events = POLLIN; // Update events to listen for input
}

void setup_data(struct self_data *my_data)
{
	printf("\033[0;32m[SETUP]\033[0m\n");
	for (int i = UDP_FDS; i < LISTENING_FDS; i++)
	{
		my_data->fds[i].events = POLLIN;
	}
	my_data->udp_socket = create_udp_sock(my_data);
	my_data->listening.socket = create_listening_tcp_sock(my_data);

	my_data->fds[UDP_FDS].fd = my_data->udp_socket;
	my_data->fds[LISTENING_FDS].fd = my_data->listening.socket;

	my_data->alive = true;
}

int create_socket(int domain, int type, int protocol, struct self_data *self_data)
{
	int sockfd = socket(domain, type, protocol);
	if (sockfd < 0)
		exit_with_error("Failed to create socket", self_data);
	return sockfd;
}

int set_nonblocking(int sockfd, struct self_data *self_data)
{
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags == -1)
		exit_with_error("Failed to get socket flags", self_data);
	return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}
