#include "hash_handling.h"

int handle_ht_remove(struct self_data *self_data, struct VAL_REMOVE_PDU remove_pdu)
{
	char *ssn_string = (char *)remove_pdu.ssn;
	int range_val = check_range(self_data, ssn_string);

	if (range_val == 0)
	{
		int num_entries = get_num_entries(self_data->hash_table);
		printf("\tRemoving SSN: {%s}\n", ssn_string);


		for (size_t i = 0; i < num_entries; i++)
		{
			if (strncmp(self_data->ssns[i], ssn_string, 12) == 0)	//check if value exists, in that case remove corresponding key from array
			{
				self_data->hash_table = ht_remove(self_data->hash_table, ssn_string);

				free(self_data->ssns[i]);		//
				for (size_t j = i; j < num_entries - 1; j++)
				{
					self_data->ssns[j] = self_data->ssns[j + 1];
				}
				self_data->ssns[num_entries - 1] = NULL;
				break;		//value is found, exit
			}
		}
		return 0;
	}
	else // val is not in nodes range, forward message
	{
		printf("\tSSN is not in range\n");
		if (send_tcp_pdu(self_data->fds[SUCCESSOR_FDS].fd, &remove_pdu, sizeof(remove_pdu)) < 0)
		{
			exit_with_error("Failed to send VAL_REMOVE_PDU to successor", self_data);
		}
		return 0;
	}

	return 0;
}

void handle_ht_insert(struct self_data *self_data, struct VAL_INSERT_PDU insert_pdu)
{
	char *ssn_string = malloc(SSN_LENGTH);
	if (!ssn_string)
	{
		exit_with_error("Failed to allocate memory for SSN", self_data);
	}

	memcpy(ssn_string, insert_pdu.ssn, SSN_LENGTH);

	struct value_pair *pair;
	int range_val = check_range(self_data, ssn_string);

	if (range_val == 0) // val is in range
	{

		printf("\tInserting SSN: {%.12s}\n", ssn_string);
		printf("\tName: {%s}", insert_pdu.name);
		printf(" Email: {%s}\n", insert_pdu.email);
		size_t num_entries = get_num_entries(self_data->hash_table);

		pair = create_value_pair(insert_pdu.name_length, insert_pdu.email_length, insert_pdu.name, insert_pdu.email);
		if (!pair)
		{
			free(ssn_string);
			exit_with_error("Failed to create value pair", self_data);
		}

		self_data->hash_table = ht_insert(self_data->hash_table, ssn_string, pair);

		if (num_entries != get_num_entries(self_data->hash_table)) // if duplicate is inserted a new element wont be made so skip adding ssn
		{
			char *new_ssn = malloc(SSN_LENGTH);
			if (!new_ssn)
			{
				free(ssn_string);
				exit_with_error("Failed to allocate memory for SSN entry", self_data);
			}
			memcpy(new_ssn, ssn_string, SSN_LENGTH);
			self_data->ssns[num_entries] = new_ssn;
		}
		return;
	}
	else
	{
		send_insert_pdu_tcp(insert_pdu, self_data, SUCCESSOR_FDS);
		free(ssn_string);
		return;
	}
}

struct value_pair *create_value_pair(uint8_t name_length, uint8_t email_length, uint8_t *name, uint8_t *email)
{
	struct value_pair *pair = malloc(sizeof(struct value_pair));
	if (!pair)
	{
		exit_with_error("Failed to allocate memory for value pair", NULL);
	}

	pair->name = malloc(name_length * sizeof(uint8_t));
	if (!pair->name)
	{
		free(pair);
		exit_with_error("Failed to allocate memory for pair name", NULL);
	}

	pair->email = malloc(email_length * sizeof(uint8_t));
	if (!pair->email)
	{
		free(pair->name);
		free(pair);
		exit_with_error("Failed to allocate memory for pair email", NULL);
	}

	memcpy(pair->name, name, name_length);
	memcpy(pair->email, email, email_length);
	pair->email_length = email_length;
	pair->name_length = name_length;
	return pair;
}

int handle_ht_lookup(struct self_data *self_data, struct VAL_LOOKUP_PDU lookup_pdu)
{

	struct sockaddr_in sender_addr;

	sender_addr.sin_addr.s_addr = lookup_pdu.sender_address; // Convert sender address inet_ntoa(sender_addr.sin_addr));
	sender_addr.sin_port = lookup_pdu.sender_port;		 // Convert sender port ntohs(sender_addr.sin_port));
	sender_addr.sin_family = AF_INET;

	char *ssn_string = (char *)lookup_pdu.ssn;
	struct VAL_LOOKUP_RESPONSE_PDU response_pdu;

	memset(&response_pdu, 0, sizeof(struct VAL_LOOKUP_RESPONSE_PDU)); // set response to 0 in case no val is fouund
	response_pdu.type = VAL_LOOKUP_RESPONSE;
	int range_val = check_range(self_data, ssn_string);

	printf("\tLooking up SSN: {%.12s}\n", ssn_string);
	if (range_val == 0)
	{
		printf("\tSSN is in range\n");
		void *res = ht_lookup(self_data->hash_table, ssn_string);

		if (res != NULL)
		{
			printf("\tSSN found\n");
			struct value_pair *pair = (struct value_pair *)res;
			response_pdu.email = pair->email;
			response_pdu.name = pair->name;
			response_pdu.email_length = pair->email_length;
			response_pdu.name_length = pair->name_length;
			memcpy(response_pdu.ssn, lookup_pdu.ssn, SSN_LENGTH);
			send_lookup_response_pdu_udp(response_pdu, self_data, sender_addr);
		}
		else
			printf("\tSSN not found\n");

		return 0;
	}
	else
	{
		printf("\tSSN is not in range\n");
		if (send_tcp_pdu(self_data->fds[SUCCESSOR_FDS].fd, &lookup_pdu, sizeof(lookup_pdu)) < 0)
		{
			exit_with_error("Failed to send VAL_LOOKUP_PDU to successor", self_data);
		}
		return 0;
	}
}

int check_range(struct self_data *self_data, char *ssn)
{
	hash_t val = hash_ssn(ssn);
	if (val >= self_data->range_start && val <= self_data->range_end) // check hash if range is withing range
		return 0;

	return 1;
}

void update_range(struct NET_NEW_RANGE_PDU range_pdu, struct self_data *self_data)
{
	struct NET_NEW_RANGE_RESPONSE_PDU response_pdu;
	response_pdu.type = NET_NEW_RANGE_RESPONSE;
	int reciever = 0;

	if (range_pdu.range_start < self_data->range_start) // if the range start is less than current range, send to predes
	{
		self_data->range_start = range_pdu.range_start;
		reciever = PREDECESSOR_FDS;
	}
	else if (range_pdu.range_end > self_data->range_end)
	{
		self_data->range_end = range_pdu.range_end;
		reciever = SUCCESSOR_FDS;
	}
	if (send_tcp_pdu(self_data->fds[reciever].fd, &response_pdu, sizeof(response_pdu)) < 0)
	{
		exit_with_error("Failed to send NET_NEW_RANGE_RESPONSE_PDU", self_data);
		return;
	}
}
void send_new_range_and_entries(struct self_data *self_data, uint32_t old_succ_adr, uint16_t old_succ_port)
{
	struct NET_JOIN_RESPONSE_PDU response;
	uint8_t middle_point;

	middle_point = (self_data->range_end - self_data->range_start) / 2 + self_data->range_start; // Calculate new range
	response.range_start = middle_point + 1;
	response.range_end = self_data->range_end;
	response.next_address = old_succ_adr;
	response.next_port = old_succ_port;
	response.type = NET_JOIN_RESPONSE;

	self_data->range_end = middle_point;

	if (send_tcp_pdu(self_data->fds[SUCCESSOR_FDS].fd, &response, sizeof(response)) < 0) // send response
	{
		exit_with_error("Failed to send NET_JOIN_RESPONSE_PDU", self_data);
	}

	int num_entries = get_num_entries(self_data->hash_table);
	struct VAL_INSERT_PDU insert_pdu;
	insert_pdu.type = VAL_INSERT;

	for (int i = 0; i < num_entries;)
	{
		if (check_range(self_data, self_data->ssns[i]) != 0) // see if current elements are in new range, if not send them
		{
			void *res = ht_lookup(self_data->hash_table, self_data->ssns[i]);
			if (res == NULL)
			{
				fprintf(stderr, "Failed to find value for key %s when it should exist\n", self_data->ssns[i]);
				return;
			}

			struct value_pair *pair = (struct value_pair *)res;
			insert_pdu.email_length = pair->email_length; // create pdu
			insert_pdu.name_length = pair->name_length;
			insert_pdu.email = pair->email;
			insert_pdu.name = pair->name;
			memcpy(insert_pdu.ssn, self_data->ssns[i], SSN_LENGTH);

			send_insert_pdu_tcp(insert_pdu, self_data, SUCCESSOR_FDS); // send it

			self_data->hash_table = ht_remove(self_data->hash_table, self_data->ssns[i]); // remove value from current table
			free(self_data->ssns[i]);						      // and free the key

			for (size_t j = i; j < num_entries - 1; j++) // fix gaps in array
			{
				self_data->ssns[j] = self_data->ssns[j + 1];
			}

			self_data->ssns[num_entries - 1] = NULL;
			num_entries--;
		}
		else
			i++;
	}
}

void send_all_entries(struct self_data *self_data, int fd)
{
	int num_entries = get_num_entries(self_data->hash_table);

	for (int i = 0; i < num_entries; i++)
	{
		struct VAL_INSERT_PDU insert_pdu;
		insert_pdu.type = VAL_INSERT;

		void *res = ht_lookup(self_data->hash_table, self_data->ssns[i]);
		if (res == NULL)
		{
			fprintf(stderr, "Failed to find value for key %s when it should exist\n", self_data->ssns[i]);
			continue;
		}

		struct value_pair *pair = (struct value_pair *)res;
		insert_pdu.email_length = pair->email_length;
		insert_pdu.name_length = pair->name_length;
		insert_pdu.email = pair->email;
		insert_pdu.name = pair->name;
		memcpy(insert_pdu.ssn, self_data->ssns[i], SSN_LENGTH);

		send_insert_pdu_tcp(insert_pdu, self_data, fd);
	}
	printf("\tFreeing memory\n");
	for (int i = 0; i < num_entries; i++)
	{
		self_data->hash_table = ht_remove(self_data->hash_table, self_data->ssns[i]);
		free(self_data->ssns[i]);
	}

	ht_destroy(self_data->hash_table);
}

/**
 * @brief Frees the memory allocated for a value pair.
 *
 * This function deallocates the memory used by a value pair, including its name and email fields.
 * If either of these fields is non-NULL, the corresponding memory is freed.
 * Finally, the memory for the value_pair structure itself is freed.
 *
 * @param value A pointer to the value pair that needs to be freed. This pointer must point to a valid memory block previously allocated.
 *
 * @return void
 *
 * @note This function performs a null-check on the input pointer to ensure it does not attempt to free memory if the pointer is NULL.
 */
void free_value_pair(void *value)
{
	if (!value)
		return; // Nothing to free if the pointer is NULL

	struct value_pair *pair = (struct value_pair *)value;

	if (pair->name)
		free(pair->name);

	if (pair->email)
		free(pair->email);

	free(pair);
}

void send_insert_pdu_tcp(const struct VAL_INSERT_PDU pdu, struct self_data *self_data, int fd)
{

	size_t pdu_size = 1 + SSN_LENGTH + 1 + pdu.name_length + 1 + pdu.email_length; // Get the size of the buffer

	uint8_t *send_buffer = malloc(pdu_size);
	if (!send_buffer)
	{
		perror("malloc");
		free(pdu.name);
		free(pdu.email);
		return;
	}

	size_t send_offset = 0;
	send_buffer[send_offset++] = pdu.type;
	memcpy(&send_buffer[send_offset], pdu.ssn, SSN_LENGTH);

	send_offset += SSN_LENGTH;
	send_buffer[send_offset++] = pdu.name_length;
	memcpy(&send_buffer[send_offset], pdu.name, pdu.name_length);

	send_offset += pdu.name_length;
	send_buffer[send_offset++] = pdu.email_length;
	memcpy(&send_buffer[send_offset], pdu.email, pdu.email_length);

	printf("\tSending insert pdu to");
	if (fd == SUCCESSOR_FDS)
		printf(" successor");
	else if (fd == PREDECESSOR_FDS)
		printf(" predecessor");
	else
		printf(" unknown");
	printf("\t :: SSN: {%.12s}\n", pdu.ssn);
	if (send_tcp_pdu(self_data->fds[fd].fd, send_buffer, pdu_size) < 0)
	{
		fprintf(stderr, "Failed to send VAL_INSERT_PDU to successor\n");
	}

	// Free the send buffer
	free(send_buffer);
}

void send_lookup_response_pdu_udp(const struct VAL_LOOKUP_RESPONSE_PDU pdu, struct self_data *self_data, struct sockaddr_in send_addr)
{

	size_t pdu_size = 1 + SSN_LENGTH + 1 + pdu.name_length + 1 + pdu.email_length; // Get the size of the buffer

	uint8_t *send_buffer = malloc(pdu_size);

	if (!send_buffer)
	{
		perror("malloc");
		free(pdu.name);
		free(pdu.email);
		return;
	}

	size_t send_offset = 0; // insert elements into buffer

	send_buffer[send_offset++] = pdu.type;
	memcpy(&send_buffer[send_offset], pdu.ssn, SSN_LENGTH);
	printf("\tSending lookup response pdu to {%.12s}\n", pdu.ssn);

	send_offset += SSN_LENGTH;
	send_buffer[send_offset++] = pdu.name_length;
	memcpy(&send_buffer[send_offset], pdu.name, pdu.name_length);
	printf("\tName: %s\n", pdu.name);

	send_offset += pdu.name_length;
	send_buffer[send_offset++] = pdu.email_length;
	memcpy(&send_buffer[send_offset], pdu.email, pdu.email_length);
	printf("\tEmail: %s\n", pdu.email);

	if (send_udp_pdu(self_data->fds[UDP_FDS].fd, send_addr, send_buffer, pdu_size) < 0) // send
	{
		exit_with_error("Failed to send VAL_LOOKUP_PDU to tracker", self_data);
	}

	free(send_buffer);
}
