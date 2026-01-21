#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SSN_LENGTH 12
#define VAL_INSERT 100

struct VAL_INSERT_PDU
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

	// Initialize three different VAL_INSERT_PDUs with different values
	struct VAL_INSERT_PDU pdu1 = {VAL_INSERT, "123456789012", 5, (uint8_t *)"Alice", 13, (uint8_t *)"alice@mail.com"};
	struct VAL_INSERT_PDU pdu2 = {VAL_INSERT, "987654321098", 3, (uint8_t *)"Bob", 11, (uint8_t *)"bob@mail.com"};
	struct VAL_INSERT_PDU pdu3 = {VAL_INSERT, "555555555555", 4, (uint8_t *)"John", 12, (uint8_t *)"john@mail.com"};

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

	// Serialize and send the first PDU
	uint8_t *buffer1 = NULL;
	size_t buffer_size1 = serialize_val_insert_pdu(&pdu1, &buffer1);
	if (sendto(sock, buffer1, buffer_size1, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
	{
		perror("sendto");
		free(buffer1);
		close(sock);
		exit(EXIT_FAILURE);
	}
	printf("VAL_INSERT_PDU 1 sent to 0.0.0.0:%d\n", port);
	free(buffer1);

	// Serialize and send the second PDU
	uint8_t *buffer2 = NULL;
	size_t buffer_size2 = serialize_val_insert_pdu(&pdu2, &buffer2);
	if (sendto(sock, buffer2, buffer_size2, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
	{
		perror("sendto");
		free(buffer2);
		close(sock);
		exit(EXIT_FAILURE);
	}
	printf("VAL_INSERT_PDU 2 sent to 0.0.0.0:%d\n", port);
	free(buffer2);

	// Serialize and send the third PDU
	uint8_t *buffer3 = NULL;
	size_t buffer_size3 = serialize_val_insert_pdu(&pdu3, &buffer3);
	if (sendto(sock, buffer3, buffer_size3, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
	{
		perror("sendto");
		free(buffer3);
		close(sock);
		exit(EXIT_FAILURE);
	}
	printf("VAL_INSERT_PDU 3 sent to 0.0.0.0:%d\n", port);
	free(buffer3);

	// Clean up and close the socket
	close(sock);

	return 0;
}