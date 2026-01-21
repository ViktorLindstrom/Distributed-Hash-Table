Distributed Hash Table (DHT)
This project implements the node part of a distributed hash table designed for load balancing and data redundancy across a dynamic network of nodes. The system consists of nodes and a tracker: nodes store and manage key–value data entries, while the tracker provides node discovery and STUN-like public address lookup to enable network connectivity.
The DHT supports node insertion and removal, dynamic hash range redistribution, and distributed insertion, lookup, and deletion of data entries using a defined PDU-based protocol over UDP and TCP. The implementation follows a state-machine-based node design and ensures no data loss as nodes join or leave the network.
This project was developed collaboratively with Noel Hedlund.
