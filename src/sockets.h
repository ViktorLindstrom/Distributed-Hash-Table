#ifndef SOCKETS_H
#define SOCKETS_H

#include "util.h"
#include "pdu.h"
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <netinet/in.h>
#include <poll.h>
#include "hashtable.h"
#include <sys/fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <poll.h>
#include <netinet/in.h>
#ifndef SOCKET_FUNCTIONS_H
#define SOCKET_FUNCTIONS_H

// Forward declarations of required structures
struct connection_point;
struct self_data;
// FD indixes
#define UDP_FDS 0
#define SUCCESSOR_FDS 1
#define PREDECESSOR_FDS 2
#define LISTENING_FDS 3
// Function Declarations

/**
 *  @brief Creates a TCP or UDP socket.
 * @param type The socket type (e.g., SOCK_STREAM for TCP).
 * @param protocol The protocol (usually 0).
 * @return int Socket file descriptor on success, exits on failure.
 */
int create_socket(int domain, int type, int protocol, struct self_data *self_data);

/**
 * @brief Sets a socket to non-blocking mode.
 * @param sockfd The socket file descriptor.
 * @return int 0 on success, exits on failure.
 */
int set_nonblocking(int sockfd, struct self_data *self_data);

/**
 * @brief Sends a UDP Protocol Data Unit (PDU).
 * @param conn_point Pointer to the connection_point structure.
 * @param pdu Pointer to the data to be sent.
 * @param pdu_size Size of the data to be sent.
 * @return int 0 on success, -1 on failure.
 */
int send_udp_pdu(int socket, struct sockaddr_in dest, const void *pdu, size_t pdu_size);

/**
 * @brief Sends a TCP Protocol Data Unit (PDU).
 * @param sockfd The socket file descriptor.
 * @param pdu Pointer to the data to be sent.
 * @param pdu_size Size of the data to be sent.
 * @return int 0 on success, -1 on failure.
 */
int send_tcp_pdu(int sockfd, const void *pdu, size_t pdu_size);

/**
 * @brief Receives data from a socket using poll to wait for input readiness.
 * @param fds Pointer to an array of pollfd structures.
 * @param fd Index of the file descriptor in the poll array.
 * @param sockfd The socket file descriptor.
 * @param pdu Pointer to buffer where received data will be stored.
 * @param pdu_size Size of the buffer.
 * @return int 0 on success, -1 on failure.
 */
int receive_from(struct pollfd *fds, int fd, int sockfd, void *pdu, size_t pdu_size);

/**
 * @brief Creates a UDP socket and binds it.
 * @param self_data Pointer to the self_data structure for error handling.
 * @return int Socket file descriptor on success, exits on failure.
 */
int create_udp_sock(struct self_data *self_data);

/**
 * @brief Creates a destination address structure.
 * @param ip The IP address as a string.
 * @param port The port number.
 * @param self_data Pointer to the self_data structure for error handling.
 * @return struct sockaddr_in The created destination address structure.
 */
struct sockaddr_in create_destination_addr(const char *ip, uint16_t port, struct self_data *self_data);

/**
 * @brief Sets up the data structures and sockets for the node.
 * @param my_data Pointer to the self_data structure.
 */
void setup_data(struct self_data *my_data);

/**
 * @brief Accepts an incoming TCP connection.
 * @param my_data Pointer to the self_data structure.
 * @param node_to_accept The type of node to accept (e.g., "predecessor" or "successor").
 */
void accept_predecessor_connection(struct self_data *my_data);

/**
 * @brief Creates a listening TCP socket.
 * @param self_data Pointer to the self_data structure for error handling.
 * @return int Socket file descriptor on success, exits on failure.
 */
int create_listening_tcp_sock(struct self_data *self_data);

/**
 * @brief Receives data from a TCP connection.
 * @param my_data Pointer to the self_data structure.
 * @param node_to_receive The type of node to receive data from (e.g., "predecessor" or "successor").
 * @param buffer Pointer to buffer where received data will be stored.
 * @param buffer_size Size of the buffer.
 */
void receive_tcp_data(struct self_data *my_data, char *node_to_receive, char *buffer, size_t buffer_size);

/**
 * @brief Connects to the successor node via TCP.
 * @param self_data Pointer to the self_data structure.
 */
void connect_to_tcp(struct self_data *self_data);

#endif // SOCKET_FUNCTIONS_H

#endif // SOCKETS_H