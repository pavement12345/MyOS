#include "netdev.h"

#define NETDEV_ETHERNET_HEADER_SIZE 14U
#define NETDEV_ETHERNET_FCS_SIZE 4U
#define NETDEV_ETHERNET_FRAME_MAX \
    (NETDEV_MTU_MAX + NETDEV_ETHERNET_HEADER_SIZE + NETDEV_ETHERNET_FCS_SIZE)

static struct netdev *devices[NETDEV_MAX_DEVICES];
static uint32_t device_count;

static int name_is_valid(const char *name) {
    if (name == 0 || *name == '\0') {
        return 0;
    }
    for (uint32_t i = 0; i < NETDEV_NAME_MAX; ++i) {
        if (name[i] == '\0') {
            return 1;
        }
    }
    return 0;
}

int netdev_register(struct netdev *device) {
    if (device == 0 || !name_is_valid(device->name) || device->ops == 0 ||
        device->ops->start == 0 || device->ops->stop == 0 ||
        device->ops->transmit == 0 || device->ops->receive == 0 ||
        device->mtu < 576U || device->mtu > NETDEV_MTU_MAX ||
        device->registered || device_count >= NETDEV_MAX_DEVICES) {
        return -1;
    }

    for (uint32_t i = 0; i < device_count; ++i) {
        if (devices[i] == device) {
            return -1;
        }
    }

    device->link_up = false;
    device->registered = true;
    devices[device_count++] = device;
    return 0;
}

uint32_t netdev_count(void) {
    return device_count;
}

struct netdev *netdev_get(uint32_t index) {
    return index < device_count ? devices[index] : 0;
}

int netdev_start(struct netdev *device) {
    if (device == 0 || !device->registered || device->ops == 0) {
        return -1;
    }
    if (device->link_up) {
        return 0;
    }
    if (device->ops->start(device) != 0) {
        device->link_up = false;
        return -1;
    }
    device->link_up = true;
    return 0;
}

void netdev_stop(struct netdev *device) {
    if (device != 0 && device->registered && device->link_up) {
        device->ops->stop(device);
        device->link_up = false;
    }
}

int netdev_transmit(struct netdev *device, const void *frame, uint32_t length) {
    if (device == 0 || !device->registered || !device->link_up || frame == 0 ||
        length < NETDEV_ETHERNET_HEADER_SIZE ||
        length > (uint32_t)device->mtu + NETDEV_ETHERNET_HEADER_SIZE +
                     NETDEV_ETHERNET_FCS_SIZE) {
        return -1;
    }
    return device->ops->transmit(device, frame, length);
}

int netdev_receive(struct netdev *device, void *frame, uint32_t capacity,
                   uint32_t *received_length) {
    if (received_length != 0) {
        *received_length = 0;
    }
    if (device == 0 || !device->registered || !device->link_up || frame == 0 ||
        received_length == 0 || capacity < NETDEV_ETHERNET_HEADER_SIZE ||
        capacity > NETDEV_ETHERNET_FRAME_MAX) {
        return -1;
    }
    return device->ops->receive(device, frame, capacity, received_length);
}
