#ifndef HASH_H
#define HASH_H

#include <inttypes.h>
#include <stddef.h>  // For size_t

// Define the hash_t type as a uint8_t
#define hash_t uint8_t

/**
 * @brief Hashes a Social Security Number (SSN).
 * 
 * This function takes an SSN (in string format) and returns a hash value.
 * The hashing algorithm used is intended to produce a small, unique hash
 * value (uint8_t) for each SSN string.
 *
 * @param ssn A string containing the SSN to be hashed (must be in the correct format).
 * @return A uint8_t hash value corresponding to the SSN.
 */
hash_t hash_ssn(char* ssn);

#endif  // HASH_H
