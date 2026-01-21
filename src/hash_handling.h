#ifndef HASH_HANDLING_H
#define HASH_HANDLING_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include "c_node.h"
#include "hashtable.h"

// Struct Definitions
struct value_pair
{
	uint8_t *name;
	uint8_t name_length;
	uint8_t *email;
	uint8_t email_length;
};

// Function Prototypes

/**
 * @brief Handles removal of a value from the hash table.
 *
 * @param self_data Pointer to the self_data structure.
 * @param fds Array of pollfd structures.
 * @param remove_pdu The VAL_REMOVE_PDU structure containing the removal request.
 *
 * @return 0 on success, -1 on failure.
 */
int handle_ht_remove(struct self_data *self_data, struct VAL_REMOVE_PDU remove_pdu);

/**
 * @brief Handles insertion of a value into the hash table.
 *
 * @param self_data Pointer to the self_data structure.
 * @param fds Array of pollfd structures.
 * @param insert_pdu The VAL_INSERT_PDU structure containing the insertion data.
 *
 * @return 0 on success, -1 on failure.
 */
void handle_ht_insert(struct self_data *self_data, struct VAL_INSERT_PDU insert_pdu);

/**
 * @brief Creates a value_pair structure with the given name and email.
 *
 * @param name_length Length of the name.
 * @param email_length Length of the email.
 * @param name Pointer to the name data.
 * @param email Pointer to the email data.
 *
 * @return Pointer to the newly created value_pair structure, or NULL on failure.
 */
struct value_pair *create_value_pair(uint8_t name_length, uint8_t email_length, uint8_t *name, uint8_t *email);

/**
 * @brief Handles lookup of a value in the hash table.
 *
 * @param self_data Pointer to the self_data structure.
 * @param lookup_pdu The VAL_LOOKUP_PDU structure containing the lookup request.
 *
 * @return 0 on success, -1 on failure.
 */
int handle_ht_lookup(struct self_data *self_data, struct VAL_LOOKUP_PDU lookup_pdu);

/**
 * @brief Checks if the hash value of a given SSN is within a specified range.
 *
 * @param self_data A pointer to the `self_data` structure that holds the range
 *                  information (`range_start` and `range_end`).
 * @param ssn A pointer to a character array (string) representing the SSN to be checked.
 *
 * @return
 * - 0 if the hash value of the SSN is between the range start and range end.
 * - -1 if the hash value of the SSN is less than the range start.
 * - 1 if the hash value of the SSN is greater than the range end.
 */
int check_range(struct self_data *self_data, char *ssn);

/**
 * @brief Updates the range of the current node based on the received range in the NET_NEW_RANGE_PDU.
 *
 * This function processes the range information received from another node, and updates the range of the current node accordingly.
 * It will adjust the range start or end of the current node if the received range is smaller or larger than the node's current range, respectively.
 * After updating the range, it sends a NET_NEW_RANGE_RESPONSE_PDU back to the appropriate node (predecessor or successor).
 *
 * @param range_pdu The NET_NEW_RANGE_PDU containing the new range information to be applied to the current node.
 * @param self_data Pointer to the current node's data, including the current range and communication file descriptors.
 *
 * @return void
 *
 * @note This function assumes that the range adjustments and communication are handled with a valid successor or predecessor.
 */

void update_range(struct NET_NEW_RANGE_PDU range_pdu, struct self_data *self_data);

/**
 * @brief Sends range and hash table entries to the new successor node after receiving a NET_JOIN_PDU.
 *
 * This function is responsible for handling the process of transferring hash table entries and range information
 * to the new successor node after a successful join operation. The function calculates the midpoint of the current
 * range and updates the range for the new node. It also sends a `NET_JOIN_RESPONSE_PDU` to the new successor to notify
 * it of the updated range. Then, it iterates through the current hash table and sends any entries that fall outside
 * the current node's range to the new successor node. After sending an entry, it removes the entry from the local hash
 * table and shifts the corresponding SSN array to maintain its integrity.
 *
 * @param self_data A pointer to the current node's data, which includes the range, SSNs, hash table, and file descriptors.
 * @param old_succ_adr A 32 bit value corresponding to the address of the old successor node
 * @param old_succ_port a 16 bit value corresponding to the port of the old successor node
 * @note This function assumes that the new successor is already set and the appropriate file descriptor for communication
 *       with the new successor is available in `self_data->fds[SUCCESSOR_FDS]`.
 *
 * @note The range update is done by calculating the midpoint of the current range, and entries outside the current
 *       node's range are forwarded to the new successor node. This function also handles the removal and shifting
 *       of entries from the local hash table and SSN array.
 */
void send_new_range_and_entries(struct self_data *self_data, uint32_t old_succ_adr, uint16_t old_succ_port);

/**
 * send_all_entries - Sends all hash table entries to a specified TCP connection. Used when exiting network and sending all entries is needed.
 * Destroys hashtable in process.
 *
 * @self_data: Pointer to the structure holding the hash table and metadata.
 * @fd: Index of the file descriptor in self_data->fds to send data to.
 *
 * This function iterates through the hash table, formats each entry into a
 * VAL_INSERT_PDU, and sends it over the specified TCP connection. After
 * successfully sending an entry, it removes the entry from the hash table and
 * frees the associated memory.
 */
void send_all_entries(struct self_data *self_data, int fd);

/**
 * @brief Sends an insert PDU (Protocol Data Unit) using TCP.
 * 
 * This function creates a buffer containing the data from the VAL_INSERT_PDU structure,
 * formats it according to the required protocol, and sends it to a successor node over
 * a TCP connection.
 * 
 * @param pdu The VAL_INSERT_PDU structure containing the data to be sent.
 * @param self_data A pointer to the self_data structure, which contains information about
 *        the current node, including file descriptors for TCP/UDP connections.
 * @param fd The file descriptor which says where to send data, successor or predecessor
 */
void send_insert_pdu_tcp(const struct VAL_INSERT_PDU pdu, struct self_data *self_data, int fd);

/**
 * @brief Sends a lookup response PDU using UDP.
 * 
 * This function creates a buffer containing the data from the VAL_LOOKUP_RESPONSE_PDU structure,
 * formats it according to the required protocol, and sends it to a specified address over
 * a UDP connection.
 * 
 * @param pdu The VAL_LOOKUP_RESPONSE_PDU structure containing the response data to be sent.
 * @param self_data A pointer to the self_data structure, which contains information about
 *        the current node, including file descriptors for TCP/UDP connections.
 * @param send_addr The address to which the PDU should be sent over UDP.
 */
void send_lookup_response_pdu_udp(const struct VAL_LOOKUP_RESPONSE_PDU pdu, struct self_data *self_data, struct sockaddr_in send_addr);

/**
 * @brief Frees the memory allocated for a value pair.
 *
 * This function deallocates the memory used by a value pair, including its `name` and `email` fields.
 * If either of these fields is non-NULL, the corresponding memory is freed.
 * Finally, the memory for the `value_pair` structure itself is freed.
 *
 * @param value A pointer to the value pair that needs to be freed. This pointer must point to a valid memory block previously allocated.
 *
 * @return void
 *
 * @note This function performs a null-check on the input pointer to ensure it does not attempt to free memory if the pointer is NULL.
 */
void free_value_pair(void *value);
#endif // HASH_HANDLING_H
