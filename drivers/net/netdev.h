#ifndef MYOS_NETDEV_H
#define MYOS_NETDEV_H

#include <stdbool.h>
#include <stdint.h>

#define NETDEV_NAME_MAX 16U
#define NETDEV_MAX_DEVICES 8U
#define NETDEV_MTU_MAX 1500U
#define NETDEV_ETHERNET_HEADER_SIZE 14U
#define NETDEV_ETHERNET_FCS_SIZE 4U
#define NETDEV_ETHERNET_FRAME_MAX \
    (NETDEV_MTU_MAX + NETDEV_ETHERNET_HEADER_SIZE + NETDEV_ETHERNET_FCS_SIZE)

enum netdev_error {
    NETDEV_OK = 0,
    NETDEV_ERROR = -1,
    NETDEV_INVALID = -2,
    NETDEV_FULL = -3,
    NETDEV_NOT_STARTED = -4,
    NETDEV_LINK_DOWN = -5,
    NETDEV_BAD_FRAME = -6
};

struct netdev;

/* Callbacks exchange Ethernet frames without the preamble or inter-frame gap.
 * Drivers may omit the FCS; the core accepts lengths up to MTU + header + FCS.
 * receive returns NETDEV_OK with length 0 when no frame is available, or with
 * a complete frame length when one was received. On error it must return a
 * negative value and leave received_length at zero. */
struct netdev_ops {
    int (*start)(struct netdev *device);
    void (*stop)(struct netdev *device);
    int (*transmit)(struct netdev *device, const void *frame, uint32_t length);
    int (*receive)(struct netdev *device, void *frame, uint32_t capacity,
                   uint32_t *received_length);
};

struct netdev {
    const char *name;
    uint8_t mac_address[6];
    uint16_t mtu;
    bool link_up; /* Physical/virtual carrier, not administrative state. */
    bool started;
    bool registered;
    const struct netdev_ops *ops;
    void *driver_data;
};

/* Device and ops objects must have static lifetime once registered. */
int netdev_register(struct netdev *device);
uint32_t netdev_count(void);
struct netdev *netdev_get(uint32_t index);
int netdev_start(struct netdev *device);
void netdev_stop(struct netdev *device);
int netdev_transmit(struct netdev *device, const void *frame, uint32_t length);
int netdev_receive(struct netdev *device, void *frame, uint32_t capacity,
                   uint32_t *received_length);

#endif
