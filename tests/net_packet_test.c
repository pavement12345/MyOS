#include "net_packet.h"

#include <assert.h>
#include <stdio.h>

static void put_be16(uint8_t *bytes, uint16_t value) {
    bytes[0] = (uint8_t)(value >> 8);
    bytes[1] = (uint8_t)value;
}

static void test_ethernet(void) {
    uint8_t frame[NET_PACKET_ETHERNET_HEADER_SIZE + 1U] = {0};
    struct net_ethernet_view view;
    frame[0] = 0x10U;
    frame[6] = 0x20U;
    put_be16(frame + 12U, 0x0800U);
    frame[14] = 0x45U;
    assert(net_packet_parse_ethernet(frame, sizeof(frame), &view) == NET_PACKET_OK);
    assert(view.destination[0] == 0x10U && view.source[0] == 0x20U);
    assert(view.ether_type == 0x0800U && view.payload_length == 1U);
    assert(net_packet_parse_ethernet(frame, 13U, &view) == NET_PACKET_TRUNCATED);
    assert(net_packet_parse_ethernet(0, sizeof(frame), &view) == NET_PACKET_INVALID);
    assert(net_packet_parse_ethernet(frame, sizeof(frame), 0) == NET_PACKET_INVALID);
}

static void test_arp(void) {
    uint8_t frame[NET_PACKET_ETHERNET_HEADER_SIZE + NET_PACKET_ARP_ETHERNET_IPV4_SIZE] = {0};
    struct net_ethernet_view ethernet;
    struct net_arp_ipv4_view arp;
    put_be16(frame + 12U, 0x0806U);
    uint8_t *payload = frame + NET_PACKET_ETHERNET_HEADER_SIZE;
    put_be16(payload, 1U);
    put_be16(payload + 2U, 0x0800U);
    payload[4] = 6U;
    payload[5] = 4U;
    put_be16(payload + 6U, 2U);
    payload[14] = 192U;
    payload[17] = 2U;
    assert(net_packet_parse_ethernet(frame, sizeof(frame), &ethernet) == NET_PACKET_OK);
    assert(net_packet_parse_arp_ipv4(&ethernet, &arp) == NET_PACKET_OK);
    assert(arp.operation == 2U && arp.sender_ipv4[0] == 192U &&
           arp.sender_ipv4[3] == 2U);
    assert(net_packet_parse_arp_ipv4(&ethernet, 0) == NET_PACKET_INVALID);
    ethernet.payload_length = NET_PACKET_ARP_ETHERNET_IPV4_SIZE - 1U;
    assert(net_packet_parse_arp_ipv4(&ethernet, &arp) == NET_PACKET_TRUNCATED);
    ethernet.payload_length = sizeof(frame) - NET_PACKET_ETHERNET_HEADER_SIZE;
    payload[4] = 5U;
    assert(net_packet_parse_arp_ipv4(&ethernet, &arp) == NET_PACKET_MALFORMED);
}

static void test_ipv4(void) {
    uint8_t frame[NET_PACKET_ETHERNET_HEADER_SIZE + 24U] = {0};
    struct net_ethernet_view ethernet;
    struct net_ipv4_view ip;
    put_be16(frame + 12U, 0x0800U);
    uint8_t *header = frame + NET_PACKET_ETHERNET_HEADER_SIZE;
    header[0] = 0x45U;
    put_be16(header + 2U, 24U);
    put_be16(header + 6U, 0x2001U); /* more fragments and offset one */
    header[9] = 17U;
    header[12] = 10U;
    header[15] = 1U;
    header[16] = 10U;
    header[19] = 2U;
    assert(net_packet_parse_ethernet(frame, sizeof(frame), &ethernet) == NET_PACKET_OK);
    assert(net_packet_parse_ipv4(&ethernet, &ip) == NET_PACKET_OK);
    assert(ip.protocol == 17U && ip.header_length == 20U && ip.total_length == 24U);
    assert(ip.fragment_offset == 1U && ip.more_fragments);
    assert(ip.source[0] == 10U && ip.source[3] == 1U);
    assert(ip.destination[0] == 10U && ip.destination[3] == 2U);
    assert(ip.payload_length == 4U);

    header[0] = 0x44U;
    assert(net_packet_parse_ipv4(&ethernet, &ip) == NET_PACKET_MALFORMED);
    header[0] = 0x46U;
    assert(net_packet_parse_ipv4(&ethernet, &ip) == NET_PACKET_TRUNCATED);
    header[0] = 0x45U;
    put_be16(header + 2U, 19U);
    assert(net_packet_parse_ipv4(&ethernet, &ip) == NET_PACKET_MALFORMED);
    put_be16(header + 2U, 25U);
    assert(net_packet_parse_ipv4(&ethernet, &ip) == NET_PACKET_TRUNCATED);
    ethernet.payload_length = 1U;
    assert(net_packet_parse_ipv4(&ethernet, &ip) == NET_PACKET_TRUNCATED);
    assert(net_packet_parse_ipv4(0, &ip) == NET_PACKET_INVALID);
}

int main(void) {
    test_ethernet();
    test_arp();
    test_ipv4();
    puts("network packet parser tests passed");
    return 0;
}
