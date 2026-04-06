#ifndef IP_H
#define IP_H

// IPv4 header representation
struct IPv4Header {
    int ttl;       // Time to Live
    int src;       // source address
    int dest;      // destination address
};

// IPv6 header representation
struct IPv6Header {
    int hop_limit; // IPv6 equivalent of TTL
    int src;       // source address
    int dest;      // destination address
};

#endif
