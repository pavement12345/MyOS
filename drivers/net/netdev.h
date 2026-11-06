#ifndef MYOS_NETDEV_H
#define MYOS_NETDEV_H

#include <stdbool.h>
#include <stdint.h>

#define NETDEV_NAME_MAX 16U
#define NETDEV_MAX_DEVICES 8U
#define NETDEV_MTU_MAX 1500U

struct netdev;

/* Driver operations exchange complete Ethernet frames. The networking core
 * owns protocol parsing; a NIC driver owns hardware and DMA details. */
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
    bool link_up;
    bool registered;
    const struct netdev_ops *ops;
    void *driver_data;
};

/* Static-lifetime device objects; no heap allocation is required. */
int netdev_register(struct netdev *device);
uint32_t netdev_count(void);
struct netdev *netdev_get(uint32_t index);
int netdev_start(struct netdev *device);
void netdev_stop(struct netdev *device);
int netdev_transmit(struct netdev *device, const void *frame, uint32_t length);
int netdev_receive(struct netdev *device, void *frame, uint32_t capacity,
                   uint32_t *received_length);

#endif
