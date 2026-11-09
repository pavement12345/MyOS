#ifndef MYOS_NET_PACKET_H
#define MYOS_NET_PACKET_H

#include <stdbool.h>
#include <stdint.h>

#define NET_PACKET_MAC_SIZE 6U
#define NET_PACKET_ETHERNET_HEADER_SIZE 14U
#define NET_PACKET_IPV4_MIN_HEADER_SIZE 20U
#define NET_PACKET_ARP_ETHERNET_IPV4_SIZE 28U

enum net_packet_kind {
    NET_PACKET_OTHER = 0,
    NET_PACKET_ARP,
    NET_PACKET_IPV4
};

enum net_packet_status {
    NET_PACKET_OK = 0,
    NET_PACKET_INVALID = -1,
    NET_PACKET_TRUNCATED = -2,
    NET_PACKET_MALFORMED = -3
};

/* Views borrow the caller's frame buffer; it must remain alive while used. */
struct net_ethernet_view {
    uint8_t destination[NET_PACKET_MAC_SIZE];
    uint8_t source[NET_PACKET_MAC_SIZE];
    uint16_t ether_type;
    const uint8_t *payload;
    uint32_t payload_length;
};

struct net_arp_ipv4_view {
    uint16_t operation;
    uint8_t sender_mac[NET_PACKET_MAC_SIZE];
    uint8_t sender_ipv4[4];
    uint8_t target_mac[NET_PACKET_MAC_SIZE];
    uint8_t target_ipv4[4];
};

struct net_ipv4_view {
    uint8_t source[4];
    uint8_t destination[4];
    uint8_t protocol;
    uint8_t header_length;
    uint16_t total_length;
    uint16_t fragment_offset;
    bool more_fragments;
    const uint8_t *payload;
    uint32_t payload_length;
};

int net_packet_parse_ethernet(const void *frame, uint32_t length,
                              struct net_ethernet_view *view);
int net_packet_parse_arp_ipv4(const struct net_ethernet_view *ethernet,
                              struct net_arp_ipv4_view *view);
int net_packet_parse_ipv4(const struct net_ethernet_view *ethernet,
                          struct net_ipv4_view *view);

#endif
