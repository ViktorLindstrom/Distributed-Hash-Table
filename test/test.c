#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/fcntl.h>
#include <string.h>
#include "hash_handling.h"

struct VAL_INSERT_PDU initialize_val_insert_pdu(uint8_t type, const uint8_t *ssn, const uint8_t *name, uint8_t name_length, const uint8_t *email, uint8_t email_length)
{
	struct VAL_INSERT_PDU pdu;

	// Set type
	pdu.type = type;

	// Copy SSN
	memcpy(pdu.ssn, ssn, SSN_LENGTH+1);

	// Allocate and copy name
	pdu.name_length = name_length;
	pdu.name = malloc(name_length * sizeof(uint8_t)); // Allocate memory
	if (!pdu.name)
	{
		perror("Failed to allocate memory for name");
		exit(EXIT_FAILURE);
	}
	memcpy(pdu.name, name, name_length);

	// Allocate and copy email
	pdu.email_length = email_length;
	pdu.email = malloc(email_length * sizeof(uint8_t)); // Allocate memory
	if (!pdu.email)
	{
		perror("Failed to allocate memory for email");
		free(pdu.name); // Free previously allocated memory to avoid leaks
		exit(EXIT_FAILURE);
	}
	memcpy(pdu.email, email, email_length);

	return pdu;
}

struct VAL_REMOVE_PDU initialize_val_remove_pdu(uint8_t type, const uint8_t *ssn)
{
	struct VAL_REMOVE_PDU pdu;

	// Set type
	pdu.type = type;

	// Copy SSN
	memcpy(pdu.ssn, ssn, SSN_LENGTH);

	return pdu;
}

struct VAL_LOOKUP_PDU initialize_val_lookup_pdu(uint8_t type, const uint8_t *ssn, uint32_t sender_address, uint16_t sender_port)
{
	struct VAL_LOOKUP_PDU pdu;

	// Set type
	pdu.type = type;

	// Copy SSN
	memcpy(pdu.ssn, ssn, SSN_LENGTH);

	// Set sender address and port
	pdu.sender_address = sender_address;
	pdu.sender_port = sender_port;

	return pdu;
}

void print_value_pair(struct value_pair *pair)
{
	if (pair == NULL)
	{
		printf("No value pair found.\n");
		return;
	}

	// Print Name
	printf("Name: ");
	for (int i = 0; i < pair->name_length; i++)
	{
		printf("%c", pair->name[i]);
	}
	printf("\n");

	// Print Email
	printf("Email: ");
	for (int i = 0; i < pair->email_length; i++)
	{
		printf("%c", pair->email[i]);
	}
	printf("\n");
}

int main()
{
	struct self_data *s1 = malloc(sizeof(struct self_data));
	struct self_data *s2 = malloc(sizeof(struct self_data));
	struct self_data *s3 = malloc(sizeof(struct self_data));

	struct pollfd *pollfd = NULL;
	s1->range_end = 255;
	s1->range_start = 120;
	s2->range_end = 160;
	s2->range_start = 81;
	s3->range_end = 254;
	s3->range_start = 161;
	for (int i = 0; i < 10; i++)
	{
		s1->ssns[i] = malloc(SSN_LENGTH);
	}

	s1->hash_table = ht_create(free_value_pair);
	s2->hash_table = ht_create(free_value_pair);
	s3->hash_table = ht_create(free_value_pair);

	uint8_t ssn[SSN_LENGTH] = "123456789012"; // SSN must be 12 characters
	uint8_t name[] = "John Doe";
	uint8_t email[] = "john.doe@example.com";

	uint8_t ssn2[SSN_LENGTH] = "345678901234"; // SSN must be 12 characters
	uint8_t name2[] = "Alice Johnson";
	uint8_t email2[] = "alice.johnson@example.com";

	uint8_t ssn3[SSN_LENGTH] = "456789012345"; // SSN must be 12 characters
	uint8_t name3[] = "Bob Brown";
	uint8_t email3[] = "bob.brown@example.com";

	uint8_t ssn4[SSN_LENGTH] = "567890123456"; // SSN must be 12 characters
	uint8_t name4[] = "Charlie Davis";
	uint8_t email4[] = "charlie.davis@example.com";

	uint8_t ssn5[SSN_LENGTH] = "678901234567"; // SSN must be 12 characters
	uint8_t name5[] = "Diana Green";
	uint8_t email5[] = "diana.green@example.com";

	

	struct VAL_INSERT_PDU pdu1 = initialize_val_insert_pdu(1, ssn, name, sizeof(name), email, sizeof(email));
	struct VAL_INSERT_PDU pdu3 = initialize_val_insert_pdu(1, ssn2, name2, sizeof(name2), email2, sizeof(email2));
	struct VAL_INSERT_PDU pdu4 = initialize_val_insert_pdu(1, ssn3, name3, sizeof(name3), email3, sizeof(email3));
	struct VAL_INSERT_PDU pdu5 = initialize_val_insert_pdu(1, ssn4, name4, sizeof(name4), email4, sizeof(email4));
	struct VAL_INSERT_PDU pdu6 = initialize_val_insert_pdu(1, ssn5, name5, sizeof(name5), email5, sizeof(email5));
	printf("Hash for PDU1 SSN: %u\n", hash_ssn(pdu1.ssn));
	printf("Hash for PDU3 SSN: %u\n", hash_ssn(pdu3.ssn));
	printf("Hash for PDU4 SSN: %u\n", hash_ssn(pdu4.ssn));
	printf("Hash for PDU5 SSN: %u\n", hash_ssn(pdu5.ssn));
	printf("Hash for PDU6 SSN: %u\n", hash_ssn(pdu6.ssn));

	// Print values for verification
	printf("Type: %u\n", pdu1.type);
	printf("SSN: %.12s\n", pdu1.ssn);
	printf("Name: %.*s\n", pdu1.name_length, pdu1.name);
	printf("Email: %.*s\n", pdu1.email_length, pdu1.email);

	int res = handle_ht_insert(s1, pollfd, pdu1);
	res = handle_ht_insert(s1, pollfd, pdu3);
	res = handle_ht_insert(s1, pollfd, pdu4);
	res = handle_ht_insert(s1, pollfd, pdu5);
	res = handle_ht_insert(s1, pollfd, pdu6);
	ht_print_keys(s1->hash_table);

	//s1->range_start = 186;
	printf("Nummer av entries efter inserts: %d\n", get_num_entries(s1->hash_table));
	//send_new_range_and_entries(s1);
	printf("NUM ENTRIES: %d\n", get_num_entries(s1->hash_table));
	for(int i = 0; i < 10; i++)
	{
		if(s1->ssns[i] == NULL)
			break;
		
		printf("%s\n", s1->ssns[i]);
	}
	// Cleanup
	return 0;
}
