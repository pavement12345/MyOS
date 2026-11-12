#include "net_packet.h"

#define ETHERTYPE_IPV4 0x0800U
#define ETHERTYPE_ARP 0x0806U
#define ARP_HARDWARE_ETHERNET 1U
#define ARP_PROTOCOL_IPV4 0x0800U

static uint16_t read_be16(const uint8_t *bytes) {
    return (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
}

static void copy_bytes(uint8_t *destination, const uint8_t *source,
                       uint32_t length) {
    for (uint32_t i = 0; i < length; ++i) {
        destination[i] = source[i];
    }
}

int net_packet_parse_ethernet(const void *frame, uint32_t length,
                              struct net_ethernet_view *view) {
    if (frame == 0 || view == 0) {
        return NET_PACKET_INVALID;
    }
    if (length < NET_PACKET_ETHERNET_HEADER_SIZE) {
        return NET_PACKET_TRUNCATED;
    }

    const uint8_t *bytes = (const uint8_t *)frame;
    copy_bytes(view->destination, bytes, NET_PACKET_MAC_SIZE);
    copy_bytes(view->source, bytes + NET_PACKET_MAC_SIZE, NET_PACKET_MAC_SIZE);
    view->ether_type = read_be16(bytes + 12U);
    view->payload = bytes + NET_PACKET_ETHERNET_HEADER_SIZE;
    view->payload_length = length - NET_PACKET_ETHERNET_HEADER_SIZE;
    return NET_PACKET_OK;
}

int net_packet_parse_arp_ipv4(const struct net_ethernet_view *ethernet,
                              struct net_arp_ipv4_view *view) {
    if (ethernet == 0 || view == 0) {
        return NET_PACKET_INVALID;
    }
    if (ethernet->ether_type != ETHERTYPE_ARP) {
        return NET_PACKET_INVALID;
    }
    if (ethernet->payload == 0 ||
        ethernet->payload_length < NET_PACKET_ARP_ETHERNET_IPV4_SIZE) {
        return NET_PACKET_TRUNCATED;
    }

    const uint8_t *arp = ethernet->payload;
    if (read_be16(arp) != ARP_HARDWARE_ETHERNET ||
        read_be16(arp + 2U) != ARP_PROTOCOL_IPV4 ||
        arp[4] != NET_PACKET_MAC_SIZE || arp[5] != 4U) {
        return NET_PACKET_MALFORMED;
    }
    uint16_t operation = read_be16(arp + 6U);
    if (operation != 1U && operation != 2U) {
        return NET_PACKET_MALFORMED;
    }

    view->operation = operation;
    copy_bytes(view->sender_mac, arp + 8U, NET_PACKET_MAC_SIZE);
    copy_bytes(view->sender_ipv4, arp + 14U, 4U);
    copy_bytes(view->target_mac, arp + 18U, NET_PACKET_MAC_SIZE);
    copy_bytes(view->target_ipv4, arp + 24U, 4U);
    return NET_PACKET_OK;
}

int net_packet_parse_ipv4(const struct net_ethernet_view *ethernet,
                          struct net_ipv4_view *view) {
    if (ethernet == 0 || view == 0) {
        return NET_PACKET_INVALID;
    }
    if (ethernet->ether_type != ETHERTYPE_IPV4) {
        return NET_PACKET_INVALID;
    }
    if (ethernet->payload == 0 || ethernet->payload_length < 1U) {
        return NET_PACKET_TRUNCATED;
    }

    const uint8_t *ip = ethernet->payload;
    if ((ip[0] >> 4) != 4U) {
        return NET_PACKET_MALFORMED;
    }
    uint32_t header_length = (uint32_t)(ip[0] & 0x0FU) * 4U;
    if (header_length < NET_PACKET_IPV4_MIN_HEADER_SIZE) {
        return NET_PACKET_MALFORMED;
    }
    if (ethernet->payload_length < header_length) {
        return NET_PACKET_TRUNCATED;
    }

    uint16_t total_length = read_be16(ip + 2U);
    if ((uint32_t)total_length < header_length) {
        return NET_PACKET_MALFORMED;
    }
    if ((uint32_t)total_length > ethernet->payload_length) {
        return NET_PACKET_TRUNCATED;
    }

    uint16_t fragment = read_be16(ip + 6U);
    view->protocol = ip[9];
    view->header_length = (uint8_t)header_length;
    view->total_length = total_length;
    view->fragment_offset = fragment & 0x1FFFU;
    view->more_fragments = (fragment & 0x2000U) != 0U;
    copy_bytes(view->source, ip + 12U, 4U);
    copy_bytes(view->destination, ip + 16U, 4U);
    view->payload = ip + header_length;
    view->payload_length = (uint32_t)total_length - header_length;
    return NET_PACKET_OK;
}
