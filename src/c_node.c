#include "c_node.h"

// To signal that shutdown is requested
volatile sig_atomic_t shutdown_requested = false;

/**
 * @brief Signal handler to set the shutdown flag.
 */
void set_shutdown()
{
	shutdown_requested = true;
	printf("\tShutdown requested\n");
}

/**
 * @brief Handles the VAL_INSERT PDU.
 *
 * @param buffer The buffer containing the PDU data.
 * @param bytes_received The number of bytes received in the buffer.
 * @param self_data Pointer to the self_data structure.
 * @return int The total size of the processed PDU, or -1 on error.
 */
int handle_val_insert(uint8_t *buffer, size_t bytes_received, struct self_data *self_data)
{
	printf("\033[0;32m[VAL INSERT] \033[0m");
	print_state(9);
	if (bytes_received < 1 + SSN_LENGTH + 1) // Minimum size check
	{
		fprintf(stderr, "Invalid VAL_INSERT_PDU received\n");
		return -1;
	}

	struct VAL_INSERT_PDU pdu;
	size_t offset = 0;

	// Parse fixed-length fields
	pdu.type = buffer[offset++];
	memcpy(pdu.ssn, &buffer[offset], SSN_LENGTH);
	offset += SSN_LENGTH;

	// Get name length
	pdu.name_length = buffer[offset++];
	if (offset + pdu.name_length > bytes_received)
	{
		fprintf(stderr, "Invalid VAL_INSERT_PDU received\n");
		return -1;
	}
	// allocate memory for dynamic name
	pdu.name = malloc(pdu.name_length);
	if (pdu.name == NULL)
	{
		perror("malloc");
		return -1;
	}
	// Copy name
	memcpy(pdu.name, &buffer[offset], pdu.name_length);
	offset += pdu.name_length;

	// Get email length
	pdu.email_length = buffer[offset++];
	if (offset + pdu.email_length > bytes_received)
	{
		return -1;
	}
	// allocate memory for dynamic email
	pdu.email = malloc(pdu.email_length);
	if (pdu.email == NULL)
	{
		perror("malloc");
		free(pdu.name);
		return -1;
	}
	// copy email
	memcpy(pdu.email, &buffer[offset], pdu.email_length);

	// Process the PDU
	handle_ht_insert(self_data, pdu);

	// Free dynamically allocated memory
	free(pdu.name);
	free(pdu.email);
	// Return the total size of the PDU
	return (1 + SSN_LENGTH + 1 + pdu.name_length + 1 + pdu.email_length);
}

/**
 * @brief Handles the VAL_LOOKUP PDU.
 *
 * @param pdu The VAL_LOOKUP_PDU structure.
 * @param self_data Pointer to the self_data structure.
 */
void handle_val_lookup(struct VAL_LOOKUP_PDU pdu, struct self_data *self_data)
{
	printf("\033[0;32m[VAL LOOKUP] \033[0m");
	print_state(9);
	handle_ht_lookup(self_data, pdu);
}

/**
 * @brief Handles the VAL_REMOVE PDU.
 *
 * @param pdu The VAL_REMOVE_PDU structure.
 * @param self_data Pointer to the self_data structure.
 */
void handle_val_remove(struct VAL_REMOVE_PDU pdu, struct self_data *self_data)
{
	printf("\033[0;32m[VAL DELETE] \033[0m");
	print_state(9);
	// Empty implementation
	handle_ht_remove(self_data, pdu);
}

/**
 * @brief Handles the NET_JOIN PDU.
 *
 * @param pdu The NET_JOIN_PDU structure.
 * @param self_data Pointer to the self_data structure.
 */
void handle_net_join(struct NET_JOIN_PDU pdu, struct self_data *self_data)
{
	int self_range = self_data->range_end - self_data->range_start;

	print_state(12);
	if ((self_data->predecessor.socket == 0) && (self_data->successor.socket == 0))
	{ // alone in network
		printf("\t");
		print_state(5);
		self_data->predecessor.dest_addr.sin_addr.s_addr = pdu.src_address;
		self_data->predecessor.dest_addr.sin_port = pdu.src_port;
		self_data->predecessor.dest_addr.sin_family = AF_INET;

		self_data->successor.dest_addr.sin_addr.s_addr = pdu.src_address;
		self_data->successor.dest_addr.sin_port = pdu.src_port;
		self_data->successor.dest_addr.sin_family = AF_INET;

		// Connect to sucessor
		connect_to_tcp(self_data);

		// transfer upper half
		printf("\tSending net_join_respons\n");
		send_new_range_and_entries(self_data, self_data->my_ip_addr.s_addr, self_data->listening.dest_addr.sin_port);

		// accept predecessor
		accept_predecessor_connection(self_data);
		// q6
	}
	else if (self_data->predecessor.socket == 0 || self_data->successor.socket == 0) // this should be impossible
	{
		exit_with_error("One of predecessor and successor is 0 but not both", self_data);
	}

	else if ((pdu.max_address == self_data->my_ip_addr.s_addr) && (pdu.max_port == self_data->listening.dest_addr.sin_port))
	{ // I have the largest span
		printf("\t");
		print_state(13);
		printf("\t Im the larges node, sending net_join_respons\n");
		// SEND NET_CLOSE to successor
		struct NET_CLOSE_CONNECTION_PDU close_pdu = {
		    .type = NET_CLOSE_CONNECTION,
		};
		send_tcp_pdu(self_data->successor.socket, &close_pdu, sizeof(close_pdu));

		// Connect to new successor
		// SEt successor adress and port
		uint32_t previous_successor_adress = self_data->successor.dest_addr.sin_addr.s_addr;
		uint16_t previous_successor_port = self_data->successor.dest_addr.sin_port;
		self_data->successor.dest_addr.sin_addr.s_addr = pdu.src_address;
		self_data->successor.dest_addr.sin_port = pdu.src_port;
		self_data->successor.dest_addr.sin_family = AF_INET;
		// connect function creates socket and connects
		connect_to_tcp(self_data); // connect to new successor

		send_new_range_and_entries(self_data, previous_successor_adress, previous_successor_port);
		// Send NET_JOIN_RESPONSE
		// TRANSFER UPPER HALF
	}
	// Check if im largest range
	else if (pdu.max_span < self_range)
	{ // My range is larger than span
		printf("\t");
		print_state(14);
		printf("\t I have the largest span, altering net_join\n");
		// Update pdu
		pdu.max_address = self_data->my_ip_addr.s_addr;
		pdu.max_port = self_data->listening.dest_addr.sin_port,
		pdu.max_span = self_range;

		// FORWARD PDU to successor
		send_tcp_pdu(self_data->successor.socket, &pdu, sizeof(pdu));
	}

	else
	{ // My span is smaller then pdu.max_span
		printf("\t");
		print_state(14);
		printf("\tMy span is smaller then net_join span\n");
		printf("\tMy span[%d] - received span [%d]\n", self_range, pdu.max_span);
		// FOrward PDU as is to successor!
		send_tcp_pdu(self_data->successor.socket, &pdu, sizeof(pdu));
	}
}

/**
 * @brief Handles the NET_NEW_RANGE PDU.
 *
 * @param pdu The NET_NEW_RANGE_PDU structure.
 * @param self_data Pointer to the self_data structure.
 */
void handle_net_new_range(struct NET_NEW_RANGE_PDU pdu, struct self_data *self_data)
{
	print_state(15);
	update_range(pdu, self_data);
	printf("\tNew range: %d-%d\n", self_data->range_start, self_data->range_end);
}

/**
 * @brief Handles the NET_LEAVING PDU.
 *
 * @param pdu The NET_LEAVING_PDU structure.
 * @param self_data Pointer to the self_data structure.
 */
void handle_net_leaving_pdu(struct NET_LEAVING_PDU pdu, struct self_data *self_data)
{
	print_state(16);
	printf("\tClosing connection to successor\n");
	close(self_data->successor.socket);
	if (pdu.new_address == self_data->my_ip_addr.s_addr && pdu.new_port == self_data->listening.dest_addr.sin_port)
	{ // I am the last node
		self_data->successor.socket = 0;
		self_data->fds[SUCCESSOR_FDS].fd = -1;
	}
	else
	{ // New node connecting, will overwrite socket and fd
		self_data->successor.dest_addr.sin_addr.s_addr = pdu.new_address;
		self_data->successor.dest_addr.sin_port = pdu.new_port;
		self_data->successor.dest_addr.sin_family = AF_INET;
		connect_to_tcp(self_data);
	}
}

/**
 * @brief Handles the NET_CLOSE_CONNECTION PDU.
 *
 * @param self_data Pointer to the self_data structure.
 */
void handle_net_close_connection(struct self_data *self_data)
{
	print_state(17);

	printf("\tClosing connection to predecessor\n");
	close(self_data->predecessor.socket);
	self_data->predecessor.socket = 0;
	self_data->fds[PREDECESSOR_FDS].fd = -1;

	if (self_data->range_start == 0 && self_data->range_end == 255)
		printf("\t I am the last Node\n");
	else
	{
		printf("\tAwaiting new predecessor\n");
		accept_predecessor_connection(self_data);
	}
}

/**
 * @brief Polls for incoming data.
 *
 * This function continuously checks for incoming data and processes it accordingly.
 *
 * @param self_data Pointer to the structure containing the necessary data for the function to operate.
 */
void poll_for_incoming_data(struct self_data *self_data, int time)
{
	int ret;
	uint8_t buffer[25600]; // Buffer to store incoming data (larger size)
	struct pollfd *fds = self_data->fds;

	int poll_time;
	if(time < 0)
		poll_time = 5000;
	else
		poll_time = time;

	// Poll for incoming data
	ret = poll(fds, 4, poll_time);
	if (ret < 0)
	{
		if (errno == EINTR)
		{
			// Interrupted by signal, check shutdown flag
			if (shutdown_requested)
			{ // If shutdown is requested, return into Q6 which will run shutdown procedure
				return;
			}
		}
		exit_with_error("Poll failed", self_data);
	}

	// Iterate through the file descriptors
	for (int i = 0; i < 3; i++)
	{
		if (fds[i].revents & POLLIN)
		{
			ssize_t bytes_received = recv(fds[i].fd, buffer, sizeof(buffer), 0);
			if (bytes_received < 0)
			{
				perror("Recv failed");
				// Set socket to 0 to indicate that it is closed
				if (i == PREDECESSOR_FDS)
				{
					printf("\tClosing connection to predecessor\n");
					close(self_data->predecessor.socket);
					self_data->predecessor.socket = 0;
				}
				else if (i == SUCCESSOR_FDS)
				{
					printf("\tClosing connection to successor\n");
					close(self_data->successor.socket);
					self_data->successor.socket = 0;
				}

				continue;
			}

			if (bytes_received == 0) // fds[i] closed its connection
			{
				printf("Connection closed on socket %d\n", fds[i].fd);
				close(fds[i].fd);
				fds[i].fd = -1; // Mark as closed
				continue;
			}

			// Process each PDU sequentially
			size_t offset = 0;
			while (offset < bytes_received)
			{
				uint8_t packet_type = buffer[offset];

				switch (packet_type)
				{
				case VAL_INSERT:
				{
					// Handle dynamic size fields (name, email) properly
					int size = handle_val_insert(buffer + offset, bytes_received - offset, self_data);
					if (size < 0)
					{
						printf("\033[31m\tFailed to handle VAL_INSERT_PDU\033[0m\n");
						offset = -1;
						break;
					}

					// After handling VAL_INSERT, we need to move the offset based on variable-length fields
					offset += size;
				}
				break;
					break;
				case VAL_LOOKUP:
				{
					struct VAL_LOOKUP_PDU pdu;
					memcpy(&pdu, buffer + offset, sizeof(pdu));
					handle_val_lookup(pdu, self_data);
					offset += sizeof(struct VAL_LOOKUP_PDU); // Adjust for PDU size
				}
				break;
				case VAL_REMOVE:
				{
					struct VAL_REMOVE_PDU pdu;
					memcpy(&pdu, buffer + offset, sizeof(pdu));
					handle_val_remove(pdu, self_data);
					offset += sizeof(struct VAL_REMOVE_PDU); // Adjust for PDU size
				}
				break;
				case NET_JOIN:
				{
					struct NET_JOIN_PDU pdu;
					memcpy(&pdu, buffer + offset, sizeof(pdu));
					handle_net_join(pdu, self_data);
					offset += sizeof(struct NET_JOIN_PDU); // Adjust for PDU size
				}
				break;
				case NET_NEW_RANGE:
				{
					struct NET_NEW_RANGE_PDU pdu;
					memcpy(&pdu, buffer + offset, sizeof(pdu));
					handle_net_new_range(pdu, self_data);
					offset += sizeof(struct NET_NEW_RANGE_PDU); // Adjust for PDU size
				}
				break;
				case NET_LEAVING:
				{
					struct NET_LEAVING_PDU pdu;
					memcpy(&pdu, buffer + offset, sizeof(pdu));
					handle_net_leaving_pdu(pdu, self_data);
					offset += sizeof(struct NET_LEAVING_PDU); // Adjust for PDU size
				}
				break;
				case NET_CLOSE_CONNECTION:
					handle_net_close_connection(self_data);
					offset++; // Move to next byte (no need to process further)
					break;
				default:
					printf("\033[31m\tInvalid PDU type: %u\033[0m\n", packet_type);
					offset = -1;
					break;
				}
				if (offset < 0)
				{
					printf("\033[31m\tError occured, in buffer cleared\033[0m\n");
					break;
				}
			}
		}
	}
}

/**
 * @brief Initializes the node and sends a STUN_LOOKUP PDU to the tracker.
 *
 * @param my_data Pointer to the self_data structure.
 * @param tracker_address The address of the tracker.
 * @param tracker_port The port of the tracker.
 */
void q1(struct self_data *my_data, const char *tracker_address, int tracker_port)
{
	print_state(1);

	// Create dest_addr for tracker
	my_data->tracker_addr = create_destination_addr(tracker_address, tracker_port, my_data);
	my_data->fds[UDP_FDS].fd = my_data->udp_socket;

	printf("\tNode started, Sending STUN_LOOKUP to tracker %s:%d\n", inet_ntoa(my_data->tracker_addr.sin_addr), htons(my_data->tracker_addr.sin_port));

	struct STUN_LOOKUP_PDU lookup_pdu = {.type = STUN_LOOKUP};
	if (send_udp_pdu(my_data->udp_socket, my_data->tracker_addr, &lookup_pdu, sizeof(lookup_pdu)))
		exit_with_error("Failed to send STUN_LOOKUP_PDU", my_data);
}

/**
 * @brief Receives the STUN_RESPONSE PDU and updates the node's IP address.
 *
 * @param my_data Pointer to the self_data structure.
 */
void q2(struct self_data *my_data)
{
	print_state(2);
	// Stun lookup sent in [Q1] expecting lookup response
	uint8_t in_buffer[5];
	if (receive_from(my_data->fds, UDP_FDS, my_data->udp_socket, in_buffer, sizeof(in_buffer)) >= 0)
	{
		if (in_buffer[0] != STUN_RESPONSE)
			exit_with_error("Unexpected response type", my_data);

		uint32_t address;
		memcpy(&address, in_buffer + 1, 4);
		my_data->my_ip_addr.s_addr = address;

		// Print the received address
		printf("\tReceived STUN_RESPONSE, my adress is: %s\n", inet_ntoa(my_data->my_ip_addr));
	}
	else
		exit_with_error("Failed to receive STUN_RESPONSE_PDU", my_data);
}

/**
 * @brief Sends a NET_GET_NODE_PDU to the tracker and processes the response.
 *
 * @param self_data Pointer to the self_data structure.
 * @return int 0 if no nodes are on the network, 1 if nodes are on the network.
 */
int q3(struct self_data *self_data)
{
	print_state(3);
	printf("\tSending NET_GET_NODE_PDU to %s:%d\n", inet_ntoa(self_data->tracker_addr.sin_addr), htons(self_data->tracker_addr.sin_port));

	// Set struct
	struct NET_GET_NODE_PDU get_node_pdu = {.type = NET_GET_NODE};

	// Send
	if (send_udp_pdu(self_data->udp_socket, self_data->tracker_addr, &get_node_pdu, sizeof(get_node_pdu)) < 0)
		exit_with_error("Failed to send NET_GET_NODE_PDU", self_data);

	// Wait for a response using poll
	uint8_t in_buffer[7];
	if (receive_from(self_data->fds, UDP_FDS, self_data->udp_socket, in_buffer, sizeof(in_buffer)) < 0)
		exit_with_error("Failed to receive NET_GET_NODE_RESPONSE_PDU", self_data);

	if (in_buffer[0] != NET_GET_NODE_RESPONSE)
		exit_with_error("Unexpected response type", self_data);

	// get adress and port
	uint32_t address;
	uint16_t port;
	memcpy(&address, in_buffer + 1, 4);
	memcpy(&port, in_buffer + 5, 2);

	if (address == 0) // Response is empty -> Q4
	{
		printf("\tReceived NET_GET_NODE_RESPONSE_PDU: address = 0\n");
		return 0;
	}
	else
	{ // Update the predecessor's address and port

		self_data->predecessor.dest_addr.sin_addr.s_addr = address;
		self_data->predecessor.dest_addr.sin_port = port;
		self_data->predecessor.dest_addr.sin_family = AF_INET;
		printf("\tReceived NET_GET_NODE_RESPONSE_PDU: address=%s, port=%u\n", inet_ntoa(self_data->predecessor.dest_addr.sin_addr), ntohs(self_data->predecessor.dest_addr.sin_port));
		return 1;
	}
}

/**
 * @brief Initializes the node when it is alone in the network.
 *        Sets up the hash table and the range of the node.
 *
 * @param self_data Pointer to the self_data structure containing node information.
 */
void q4(struct self_data *self_data)
{
	print_state(4);
	printf("\tAlone in network\n");
	// Create hash table
	self_data->hash_table = ht_create(NULL);
	if (self_data->hash_table == NULL)
		exit_with_error("Failed to create hash table", self_data);
	self_data->range_start = 0;
	self_data->range_end = 255;
}

/**
 * @brief Handles the process of joining the network by sending a NET_JOIN_PDU
 *        and accepting a connection from the predecessor. Updates the successor
 *        information based on the response.
 *
 * @param self_data Pointer to the self_data structure containing node information.
 */
void q7(struct self_data *self_data)
{
	print_state(7);
	// Send listening socket to accept new sucessor at (tcp)
	// then accept the predecessor on predecessor sokcet (TCP)
	// Send NET_JOIN_RESPONSE_PDU to predecessor
	struct NET_JOIN_PDU join_pdu = {
	    .type = NET_JOIN,
	    .src_address = (self_data->my_ip_addr.s_addr),
	    .src_port = (self_data->listening.dest_addr.sin_port),
	    .max_span = 0,
	    .max_address = 0,
	    .max_port = 0};

	if (send_udp_pdu(self_data->udp_socket, self_data->predecessor.dest_addr, &join_pdu, sizeof(join_pdu)) < 0)
		exit_with_error("Failed to send NET_JOIN_PDU", self_data);

	accept_predecessor_connection(self_data);
	// Should listen for incomming response from any node
	uint8_t in_buffer[9];
	receive_tcp_data(self_data, "predecessor", (char *)in_buffer, sizeof(in_buffer));
	if (in_buffer[0] != NET_JOIN_RESPONSE)
		exit_with_error("Unexpected response type", self_data);

	uint32_t address;
	uint16_t port;
	memcpy(&address, in_buffer + 1, 4);
	memcpy(&port, in_buffer + 5, 2);
	// SHOULD this  BE HTONSS AND HTONL ?!?!?!
	memset(&self_data->successor.dest_addr, 0, sizeof(self_data->successor.dest_addr));
	self_data->successor.dest_addr.sin_addr.s_addr = address;
	self_data->successor.dest_addr.sin_port = port;
	self_data->successor.dest_addr.sin_family = AF_INET;
	self_data->range_start = in_buffer[7];
	self_data->range_end = in_buffer[8];

	printf("\tGot NET_JOIN_RESPONSE_PDU: address=%s, port=%u\n", inet_ntoa(self_data->successor.dest_addr.sin_addr), ntohs(self_data->successor.dest_addr.sin_port));
	printf("\trange start: %d\n", in_buffer[7]);
	printf("\trange end: %d\n", in_buffer[8]);
}

/**
 * @brief Connects the node to the network via TCP and initializes the hash table.
 *
 * @param self_data Pointer to the self_data structure containing node information.
 */
void q8(struct self_data *self_data)
{
	print_state(8);
	connect_to_tcp(self_data);
	self_data->hash_table = ht_create(NULL);
	if (self_data->hash_table == NULL)
		exit_with_error("Failed to create hash table", self_data);
}

/**
 * @brief Continuously runs the node's main loop, sending NET_ALIVE PDUs and
 *        polling for incoming data until a shutdown is requested.
 *
 * @param self_data Pointer to the self_data structure containing node information.
 */
void q6(struct self_data *self_data)
{
	while (!shutdown_requested)
	{
		printf("\n");
		printf("\033[32m[Q6]\033[0m    Range[%d-%d]  entries[%d]\n", self_data->range_start, self_data->range_end, get_num_entries(self_data->hash_table));
		// SEND NET ALIVE
		struct NET_ALIVE_PDU alive_pdu = {.type = NET_ALIVE};
		if (send_udp_pdu(self_data->udp_socket, self_data->tracker_addr, &alive_pdu, sizeof(alive_pdu)) < 0)
			exit_with_error("Failed to send NET_ALIVE", self_data);

		// Poll for incomming data
		poll_for_incoming_data(self_data, -1);
		// Check type of data and handle accordingly
	}
}

/**
 * @brief Handles the shutdown procedure for the node, including sending
 *        NET_NEW_RANGE, NET_CLOSE_CONNECTION, and NET_LEAVING PDUs, and
 *        closing all sockets.
 *
 * @param self_data Pointer to the self_data structure containing node information.
 */
void shutdown_proceedure(struct self_data *self_data)
{
	printf("\033[32m\nShutting down\033[0m\n");
	print_state(10);
	if ((self_data->predecessor.socket == 0) && (self_data->successor.socket == 0))
		exit(EXIT_SUCCESS); // Not connected
	printf("\t");
	print_state(11);
	// Send NET_NEW_RANGE to predecessor or sucessor
	struct NET_NEW_RANGE_PDU new_range_pdu = {
	    .type = NET_NEW_RANGE,
	    .range_start = self_data->range_start,
	    .range_end = self_data->range_end,
	};
	struct NET_NEW_RANGE_RESPONSE_PDU new_range_response_pdu = {
	    .type = NET_NEW_RANGE_RESPONSE,
	};

	if (self_data->range_start == 0)
	{ // send to successor
		printf("\tSending new range to successor\n");
		send_tcp_pdu(self_data->successor.socket, &new_range_pdu, sizeof(new_range_pdu));
		// receive NET_NEW_RANGE_RESPONSE
		receive_tcp_data(self_data, "successor", (char *)&new_range_response_pdu, sizeof(new_range_response_pdu));
		printf("\t");
		print_state(18);
		print_state(18);
		printf("\tGot response -> Transfering entries to successor\n");
		send_all_entries(self_data, SUCCESSOR_FDS);
	}
	else
	{ // Send to predecessor
		printf("\tSending new range to predecessor\n");
		send_tcp_pdu(self_data->predecessor.socket, &new_range_pdu, sizeof(new_range_pdu));
		// receive NET_NEW_RANGE_RESPONSE
		receive_tcp_data(self_data, "predecessor", (char *)&new_range_response_pdu, sizeof(new_range_response_pdu));
		printf("\t");
		print_state(18);
		print_state(18);
		printf("\tGot response -> Transfering entries to predecessor\n");
		send_all_entries(self_data, PREDECESSOR_FDS);
	}

	// Get response

	// Get response
	if (new_range_response_pdu.type != NET_NEW_RANGE_RESPONSE)
		exit_with_error("Unexpected response type", self_data);
	// Prepare leaving messages
	struct NET_CLOSE_CONNECTION_PDU close_pdu = {
	    .type = NET_CLOSE_CONNECTION,
	};
	struct NET_LEAVING_PDU leaving_pdu = {
	    .type = NET_LEAVING,
	    .new_address = self_data->successor.dest_addr.sin_addr.s_addr,
	    .new_port = self_data->successor.dest_addr.sin_port,
	};

	// Exiting, dont have range -> set values to continue forwarding messages until exit is complete.
	self_data->range_end = -1;
	self_data->range_start = -1;
	poll_for_incoming_data(self_data, 300);// POll to forward messages for a second while all inserts are sent.
	//Without this poll() or a sleep() and with alarge amount of Inserts "Send_all()" to a Rust node only picks the -
	// first insert up then closes socket

	printf("\tSending NET_CLOSE to successor\n");
	send_tcp_pdu(self_data->successor.socket, &close_pdu, sizeof(close_pdu));
	printf("\tSending NET_LEAVING to predecessor\n");
	send_tcp_pdu(self_data->predecessor.socket, &leaving_pdu, sizeof(leaving_pdu));

	// Close sockets
	close_all_sockets(self_data);
	// Free hash table

	printf("\033[32mGOODBYE ;)\033[0m\n");
	exit(EXIT_SUCCESS);
}
/**
 * @brief WIll need four sockets,
 * 	  Sucessor(TCP)
 *  predecessor(TCP)
 *  tracker(UDP - PDU´s)
 *  listening port(UDP - PDU´s) - to get "net join"
 *
 * @param argc
 * @param argv (Tracker adress) (Tracker port) (Listining Port)
 * @return int
 * 				0 - Success
 * 				1 - Failure
 */

int main(int argc, char *argv[])
{
	// ------ Check input arguments ------
	if (argc != 3)
	{
		printf("Usage: %s <tracker_address> <tracker_port>\n", argv[0]);
		return 1;
	}
	// ------ allocate memory for data struct ------
	// This is validated in function below
	const char *tracker_address = argv[1];
	const int tracker_port = atoi(argv[2]);
	struct self_data *my_data = malloc(sizeof(struct self_data));
	if (!my_data)
		exit_with_error("Failed to allocate memory", my_data);

	// Set up sockets and self_data
	setup_data(my_data);

	// Set up signalr handelers to catch shutdown request
	signal(SIGINT, set_shutdown);
	signal(SIGTERM, set_shutdown);

	// ------ Initialize the node ------
	q1(my_data, tracker_address, tracker_port);
	q2(my_data);
	// ------ Q3 ------
	switch (q3(my_data))
	{
	case 0: // no nodes on network
		printf("\tNo nodes on network start\n");
		q4(my_data);
		// Q4
		break;
	case 1: // connect to nodes, Predeccesor adress (not socket) is set in my_data
		printf("\tnodes are on network\n");
		printf("\tPredecessor adress is: %s:%d\n", inet_ntoa(my_data->predecessor.dest_addr.sin_addr), ntohs(my_data->predecessor.dest_addr.sin_port));
		// NET JOIN
		q7(my_data);
		q8(my_data);
		// Q7 -> Q8
		break;
	default:
		exit_with_error("Failed to initialize node network", my_data);
		break;
	}
	// NODE is now fully connected to network
	// Q6 will run until SIGINT or SIGTERM is received
	q6(my_data);
	// Shutdown
	shutdown_proceedure(my_data);
	return 0;
}