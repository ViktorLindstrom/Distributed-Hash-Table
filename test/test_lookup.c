#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SSN_LENGTH 12
#define VAL_INSERT 100
#define VAL_LOOKUP 102
#define VAL_LOOKUP_RESPONSE 103

struct VAL_INSERT_PDU
{
	uint8_t type;
	uint8_t ssn[SSN_LENGTH];
	uint8_t name_length;
	uint8_t *name;
	uint8_t email_length;
	uint8_t *email;
};

struct VAL_LOOKUP_PDU
{
	uint8_t type;
	uint8_t ssn[SSN_LENGTH];
	uint32_t sender_address;
	uint16_t sender_port;
};

struct VAL_LOOKUP_RESPONSE_PDU
{
	uint8_t type;
	uint8_t ssn[SSN_LENGTH];
	uint8_t name_length;
	uint8_t *name;
	uint8_t email_length;
	uint8_t *email;
};

// Function to serialize the VAL_INSERT_PDU into a buffer
size_t serialize_val_insert_pdu(struct VAL_INSERT_PDU *pdu, uint8_t **buffer)
{
	size_t size = sizeof(pdu->type) + SSN_LENGTH + sizeof(pdu->name_length) +
		      pdu->name_length + sizeof(pdu->email_length) + pdu->email_length;
	*buffer = malloc(size);
	if (*buffer == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	uint8_t *ptr = *buffer;
	*ptr++ = pdu->type;
	memcpy(ptr, pdu->ssn, SSN_LENGTH);
	ptr += SSN_LENGTH;
	*ptr++ = pdu->name_length;
	memcpy(ptr, pdu->name, pdu->name_length);
	ptr += pdu->name_length;
	*ptr++ = pdu->email_length;
	memcpy(ptr, pdu->email, pdu->email_length);

	return size;
}

// Function to serialize the VAL_LOOKUP_PDU into a buffer
size_t serialize_val_lookup_pdu(struct VAL_LOOKUP_PDU *pdu, uint8_t **buffer)
{
	size_t size = sizeof(pdu->type) + SSN_LENGTH + sizeof(pdu->sender_address) + sizeof(pdu->sender_port);
	*buffer = malloc(size);
	if (*buffer == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	uint8_t *ptr = *buffer;
	*ptr++ = pdu->type;
	memcpy(ptr, pdu->ssn, SSN_LENGTH);
	ptr += SSN_LENGTH;
	memcpy(ptr, &pdu->sender_address, sizeof(pdu->sender_address));
	ptr += sizeof(pdu->sender_address);
	memcpy(ptr, &pdu->sender_port, sizeof(pdu->sender_port));

	return size;
}

// Function to deserialize the VAL_LOOKUP_RESPONSE_PDU from a buffer
int deserialize_val_lookup_response_pdu(uint8_t *buffer, struct VAL_LOOKUP_RESPONSE_PDU *pdu)
{
	uint8_t *ptr = buffer;
	pdu->type = *ptr++;
	memcpy(pdu->ssn, ptr, SSN_LENGTH);
	ptr += SSN_LENGTH;
	pdu->name_length = *ptr++;
	pdu->name = malloc(pdu->name_length);
	if (!pdu->name)
	{
		perror("malloc");
		return -1;
	}
	memcpy(pdu->name, ptr, pdu->name_length);
	ptr += pdu->name_length;
	pdu->email_length = *ptr++;
	pdu->email = malloc(pdu->email_length);
	if (!pdu->email)
	{
		perror("malloc");
		return -1;
	}
	memcpy(pdu->email, ptr, pdu->email_length);

	return 0;
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
		fprintf(stderr, "Invalid port: %d\n", port);
		exit(EXIT_FAILURE);
	}

	// Initialize a VAL_INSERT_PDU
	struct VAL_INSERT_PDU pdu1 = {VAL_INSERT, "123456789012", 5, (uint8_t *)"Alice", 13, (uint8_t *)"alice@mail.com"};

	// Create a UDP socket
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Set up the destination address
	struct sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, "0.0.0.0", &dest_addr.sin_addr) <= 0)
	{
		perror("inet_pton");
		close(sock);
		exit(EXIT_FAILURE);
	}

	// Serialize and send the VAL_INSERT_PDU
	uint8_t *buffer1 = NULL;
	size_t buffer_size1 = serialize_val_insert_pdu(&pdu1, &buffer1);
	if (sendto(sock, buffer1, buffer_size1, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
	{
		perror("sendto");
		free(buffer1);
		close(sock);
		exit(EXIT_FAILURE);
	}
	printf("VAL_INSERT_PDU sent to 0.0.0.0:%d\n", port);
	free(buffer1);

	// Prepare and send the VAL_LOOKUP_PDU
	struct VAL_LOOKUP_PDU lookup_pdu = {VAL_LOOKUP, "123456789012", inet_addr("0.0.0.0"), port};
	uint8_t *buffer2 = NULL;
	size_t buffer_size2 = serialize_val_lookup_pdu(&lookup_pdu, &buffer2);
	if (sendto(sock, buffer2, buffer_size2, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
	{
		perror("sendto");
		free(buffer2);
		close(sock);
		exit(EXIT_FAILURE);
	}
	printf("VAL_LOOKUP_PDU sent to 0.0.0.0:%d\n", port);
	free(buffer2);

	// Prepare to receive the VAL_LOOKUP_RESPONSE_PDU
	uint8_t recv_buffer[1024];
	struct sockaddr_in recv_addr;
	socklen_t recv_addr_len = sizeof(recv_addr);
	ssize_t recv_len = recvfrom(sock, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_addr, &recv_addr_len);
	if (recv_len < 0)
	{
		perror("recvfrom");
		close(sock);
		exit(EXIT_FAILURE);
	}

	// Deserialize the VAL_LOOKUP_RESPONSE_PDU
	struct VAL_LOOKUP_RESPONSE_PDU response_pdu;
	if (deserialize_val_lookup_response_pdu(recv_buffer, &response_pdu) == 0)
	{
		printf("VAL_LOOKUP_RESPONSE_PDU received:\n");
		printf("SSN: %s\n", response_pdu.ssn);
		printf("Name: %s\n", response_pdu.name);
		printf("Email: %s\n", response_pdu.email);
		free(response_pdu.name);
		free(response_pdu.email);
	}
	else
	{
		printf("Failed to deserialize VAL_LOOKUP_RESPONSE_PDU\n");
	}

	// Clean up and close the socket
	close(sock);

	return 0;
}