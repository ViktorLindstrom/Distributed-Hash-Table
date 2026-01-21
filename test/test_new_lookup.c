#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_ADDRESS "127.0.0.1"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#define SSN_LENGTH 12

#define NET_ALIVE 0
#define NET_GET_NODE 1
#define NET_GET_NODE_RESPONSE 2
#define NET_JOIN 3
#define NET_JOIN_RESPONSE 4
#define NET_CLOSE_CONNECTION 5

#define NET_NEW_RANGE 6
#define NET_LEAVING 7
#define NET_NEW_RANGE_RESPONSE 8

#define VAL_INSERT 100
#define VAL_REMOVE 101
#define VAL_LOOKUP 102
#define VAL_LOOKUP_RESPONSE 103

#define STUN_LOOKUP 200
#define STUN_RESPONSE 201

struct NET_ALIVE_PDU
{
	uint8_t type;
};

struct NET_GET_NODE_PDU
{
	uint8_t type;
};

struct NET_GET_NODE_RESPONSE_PDU
{
	uint8_t type;
	uint32_t address;
	uint16_t port;
};
#pragma pack(push, 1)
struct NET_JOIN_PDU
{
	uint8_t type;
	uint32_t src_address;
	uint16_t src_port;
	uint8_t max_span;
	uint32_t max_address;
	uint16_t max_port;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct NET_JOIN_RESPONSE_PDU
{
	uint8_t type;
	uint32_t next_address;
	uint16_t next_port;
	uint8_t range_start;
	uint8_t range_end;
};
#pragma pack(pop)
struct NET_CLOSE_CONNECTION_PDU
{
	uint8_t type;
};

struct NET_NEW_RANGE_PDU
{
	uint8_t type;
	uint8_t range_start;
	uint8_t range_end;
};

struct NET_NEW_RANGE_RESPONSE_PDU
{
	uint8_t type;
};

#pragma pack(push, 1)
struct NET_LEAVING_PDU
{
	uint8_t type;
	uint32_t new_address;
	uint16_t new_port;
};
#pragma pack(pop)

struct VAL_INSERT_PDU
{
	uint8_t type;
	uint8_t ssn[SSN_LENGTH];
	uint8_t name_length;
	uint8_t *name;
	uint8_t email_length;
	uint8_t *email;
};

struct VAL_REMOVE_PDU
{
	uint8_t type;
	uint8_t ssn[SSN_LENGTH];
};

#pragma pack(push, 1)
struct VAL_LOOKUP_PDU
{
	uint8_t type;
	uint8_t ssn[SSN_LENGTH];
	uint32_t sender_address;
	uint16_t sender_port;
};
#pragma pack(pop)

struct VAL_LOOKUP_RESPONSE_PDU
{
	uint8_t type;
	uint8_t ssn[SSN_LENGTH];
	uint8_t name_length;
	uint8_t *name;
	uint8_t email_length;
	uint8_t *email;
};

struct STUN_LOOKUP_PDU
{
	uint8_t type;
};

struct STUN_RESPONSE_PDU
{
	uint8_t type;
	uint32_t address;
};

void send_val_lookup(int sockfd, struct sockaddr_in *server_addr)
{
	// Prepare VAL_LOOKUP PDU
	struct VAL_LOOKUP_PDU pdu;
	pdu.type = VAL_LOOKUP;
	memcpy(pdu.ssn, "123456789012", SSN_LENGTH);

	// Get the sender address and port from the local socket binding
	struct sockaddr_in local_addr;
	socklen_t addr_len = sizeof(local_addr);
	if (getsockname(sockfd, (struct sockaddr *)&local_addr, &addr_len) == -1)
	{
		perror("Failed to get socket name");
		exit(EXIT_FAILURE);
	}

	pdu.sender_address = local_addr.sin_addr.s_addr;
	pdu.sender_port = local_addr.sin_port;

	// Print the sender address and port
	char ip_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &pdu.sender_address, ip_str, INET_ADDRSTRLEN);
	printf("Sender Address: %s\n", ip_str);
	printf("Sender Port: %d\n", ntohs(pdu.sender_port));

	// Serialize PDU
	size_t pdu_size = sizeof(pdu);

	if (sendto(sockfd, &pdu, pdu_size, 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) == -1)
	{
		perror("Failed to send VAL_LOOKUP PDU");
		exit(EXIT_FAILURE);
	}

	printf("VAL_LOOKUP PDU sent successfully.\n");
}

void wait_for_val_lookup_response(int sockfd)
{
	uint8_t buffer[4096];
	struct sockaddr_in from_addr;
	socklen_t from_len = sizeof(from_addr);

	// Wait for VAL_LOOKUP_RESPONSE
	ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&from_addr, &from_len);
	if (bytes_received < 0)
	{
		perror("Failed to receive data");
		exit(EXIT_FAILURE);
	}
	int offset = 0;
	printf("bytes_received: %ld\n", bytes_received);

	{
		printf("received type = %d\n", buffer[0]);

		uint8_t ssn[SSN_LENGTH];
		memcpy(ssn, buffer + 1, SSN_LENGTH);
		printf("received ssn = %.12s\n", ssn);
		offset += 1 + SSN_LENGTH;
		int name_length = buffer[offset];
		printf("name length = %d\n", buffer[offset]);

		uint8_t name[name_length];
		memcpy(name, buffer + offset + 1, name_length);
		printf("received name = %.*s\n", name_length, name);

		offset += 1 + name_length;
		int email_length = buffer[offset];
		printf("email length = %d\n", buffer[offset]);
		uint8_t email[email_length];
		memcpy(email, buffer + offset + 1, email_length);
		printf("received email = %.*s\n", email_length, email);
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int port = atoi(argv[1]);
	if (port <= 0 || port > 65535)
	{
		fprintf(stderr, "Invalid port number.\n");
		exit(EXIT_FAILURE);
	}

	// Create UDP socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1)
	{
		perror("Failed to create socket");
		exit(EXIT_FAILURE);
	}

	// Bind the socket to the local address and port
	struct sockaddr_in local_addr;
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = 0; // Let the OS pick an available port
	local_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1)
	{
		perror("Failed to bind socket");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	// Get the bound port
	socklen_t addr_len = sizeof(local_addr);
	if (getsockname(sockfd, (struct sockaddr *)&local_addr, &addr_len) == -1)
	{
		perror("Failed to get socket name");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	printf("Bound to port: %d\n", ntohs(local_addr.sin_port));

	// Define server address
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, SERVER_ADDRESS, &server_addr.sin_addr) <= 0)
	{
		perror("Invalid server address");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	// Send the VAL_LOOKUP PDU
	send_val_lookup(sockfd, &server_addr);
	wait_for_val_lookup_response(sockfd);

	// Close socket
	close(sockfd);
	return 0;
}