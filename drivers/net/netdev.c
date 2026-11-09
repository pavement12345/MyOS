#include "netdev.h"

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

static int ops_are_valid(const struct netdev_ops *ops) {
    return ops != 0 && ops->start != 0 && ops->stop != 0 &&
           ops->transmit != 0 && ops->receive != 0;
}

int netdev_register(struct netdev *device) {
    if (device == 0 || !name_is_valid(device->name) || !ops_are_valid(device->ops) ||
        device->mtu < 576U || device->mtu > NETDEV_MTU_MAX ||
        device->registered || device->started) {
        return NETDEV_INVALID;
    }
    if (device_count >= NETDEV_MAX_DEVICES) {
        return NETDEV_FULL;
    }
    for (uint32_t i = 0; i < device_count; ++i) {
        if (devices[i] == device ||
            (name_is_valid(devices[i]->name) &&
             __builtin_strcmp(devices[i]->name, device->name) == 0)) {
            return NETDEV_INVALID;
        }
    }

    device->link_up = false;
    device->started = false;
    device->registered = true;
    devices[device_count++] = device;
    return NETDEV_OK;
}

uint32_t netdev_count(void) {
    return device_count;
}

struct netdev *netdev_get(uint32_t index) {
    return index < device_count ? devices[index] : 0;
}

int netdev_start(struct netdev *device) {
    if (device == 0 || !device->registered || !ops_are_valid(device->ops)) {
        return NETDEV_INVALID;
    }
    if (device->started) {
        return NETDEV_OK;
    }
    device->link_up = false;
    if (device->ops->start(device) != 0) {
        device->started = false;
        device->link_up = false;
        return NETDEV_ERROR;
    }
    device->started = true;
    return NETDEV_OK;
}

void netdev_stop(struct netdev *device) {
    if (device != 0 && device->registered && device->started &&
        ops_are_valid(device->ops)) {
        device->ops->stop(device);
        device->started = false;
        device->link_up = false;
    }
}

static uint32_t frame_limit(const struct netdev *device) {
    return (uint32_t)device->mtu + NETDEV_ETHERNET_HEADER_SIZE +
           NETDEV_ETHERNET_FCS_SIZE;
}

int netdev_transmit(struct netdev *device, const void *frame, uint32_t length) {
    if (device == 0 || !device->registered || !ops_are_valid(device->ops) ||
        frame == 0 || length < NETDEV_ETHERNET_HEADER_SIZE ||
        length > frame_limit(device)) {
        return NETDEV_BAD_FRAME;
    }
    if (!device->started) {
        return NETDEV_NOT_STARTED;
    }
    if (!device->link_up) {
        return NETDEV_LINK_DOWN;
    }
    return device->ops->transmit(device, frame, length);
}

int netdev_receive(struct netdev *device, void *frame, uint32_t capacity,
                   uint32_t *received_length) {
    if (received_length != 0) {
        *received_length = 0;
    }
    if (device == 0 || !device->registered || !ops_are_valid(device->ops) ||
        frame == 0 || received_length == 0 ||
        capacity < NETDEV_ETHERNET_HEADER_SIZE ||
        capacity > NETDEV_ETHERNET_FRAME_MAX) {
        return NETDEV_INVALID;
    }
    if (!device->started) {
        return NETDEV_NOT_STARTED;
    }
    if (!device->link_up) {
        return NETDEV_LINK_DOWN;
    }

    uint32_t length = 0;
    int result = device->ops->receive(device, frame, capacity, &length);
    if (result != NETDEV_OK) {
        return result;
    }
    if (length == 0U) {
        return NETDEV_OK;
    }
    if (length < NETDEV_ETHERNET_HEADER_SIZE || length > capacity ||
        length > frame_limit(device)) {
        return NETDEV_BAD_FRAME;
    }
    *received_length = length;
    return NETDEV_OK;
}
