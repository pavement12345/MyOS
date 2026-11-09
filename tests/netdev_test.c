#include "netdev.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int start_result;
static int tx_calls;
static int stop_calls;
static uint32_t receive_length;
static int receive_result;

static int mock_start(struct netdev *device) {
    if (start_result == 0) {
        device->link_up = true;
    }
    return start_result;
}

static void mock_stop(struct netdev *device) {
    (void)device;
    ++stop_calls;
}

static int mock_transmit(struct netdev *device, const void *frame, uint32_t length) {
    (void)device;
    (void)frame;
    (void)length;
    ++tx_calls;
    return NETDEV_OK;
}

static int mock_receive(struct netdev *device, void *frame, uint32_t capacity,
                        uint32_t *received) {
    (void)device;
    (void)frame;
    (void)capacity;
    *received = receive_length;
    return receive_result;
}

static const struct netdev_ops mock_ops = {
    .start = mock_start,
    .stop = mock_stop,
    .transmit = mock_transmit,
    .receive = mock_receive
};

int main(void) {
    static struct netdev devices[NETDEV_MAX_DEVICES + 1U];
    static char names[NETDEV_MAX_DEVICES + 1U][NETDEV_NAME_MAX];
    uint8_t frame[NETDEV_ETHERNET_FRAME_MAX] = {0};
    uint32_t length = 123U;

    assert(netdev_count() == 0U);
    assert(netdev_get(0U) == 0);
    assert(netdev_register(0) == NETDEV_INVALID);
    devices[0] = (struct netdev){ .name = "", .mtu = 1500U, .ops = &mock_ops };
    assert(netdev_register(&devices[0]) == NETDEV_INVALID);
    devices[0] = (struct netdev){ .name = "bad-mtu", .mtu = 575U, .ops = &mock_ops };
    assert(netdev_register(&devices[0]) == NETDEV_INVALID);

    for (uint32_t i = 0; i < NETDEV_MAX_DEVICES; ++i) {
        (void)snprintf(names[i], sizeof(names[i]), "eth%u", i);
        devices[i] = (struct netdev){ .name = names[i], .mtu = 1500U, .ops = &mock_ops };
        assert(netdev_register(&devices[i]) == NETDEV_OK);
    }
    assert(netdev_count() == NETDEV_MAX_DEVICES);
    assert(netdev_get(NETDEV_MAX_DEVICES) == 0);
    assert(netdev_register(&devices[0]) == NETDEV_INVALID);
    devices[NETDEV_MAX_DEVICES] = (struct netdev){
        .name = "overflow", .mtu = 1500U, .ops = &mock_ops
    };
    assert(netdev_register(&devices[NETDEV_MAX_DEVICES]) == NETDEV_FULL);

    struct netdev *device = netdev_get(0U);
    assert(device == &devices[0]);
    assert(netdev_transmit(device, frame, 60U) == NETDEV_NOT_STARTED);
    assert(netdev_receive(device, frame, sizeof(frame), &length) == NETDEV_NOT_STARTED);
    assert(length == 0U);

    start_result = -1;
    assert(netdev_start(device) == NETDEV_ERROR);
    assert(!device->started && !device->link_up);
    start_result = 0;
    assert(netdev_start(device) == NETDEV_OK);
    assert(device->started && device->link_up);
    assert(netdev_start(device) == NETDEV_OK);

    assert(netdev_transmit(device, frame, 13U) == NETDEV_BAD_FRAME);
    assert(netdev_transmit(device, frame, NETDEV_ETHERNET_FRAME_MAX + 1U) == NETDEV_BAD_FRAME);
    assert(netdev_transmit(device, frame, 60U) == NETDEV_OK);
    assert(tx_calls == 1);

    receive_result = NETDEV_OK;
    receive_length = 0U;
    length = 123U;
    assert(netdev_receive(device, frame, sizeof(frame), &length) == NETDEV_OK);
    assert(length == 0U); /* Poll with no available packet. */
    receive_length = NETDEV_ETHERNET_HEADER_SIZE - 1U;
    assert(netdev_receive(device, frame, sizeof(frame), &length) == NETDEV_BAD_FRAME);
    assert(length == 0U);
    receive_length = sizeof(frame) + 1U;
    assert(netdev_receive(device, frame, sizeof(frame), &length) == NETDEV_BAD_FRAME);
    assert(length == 0U);
    receive_length = 60U;
    assert(netdev_receive(device, frame, 60U, &length) == NETDEV_OK);
    assert(length == 60U);
    receive_result = NETDEV_ERROR;
    length = 123U;
    assert(netdev_receive(device, frame, sizeof(frame), &length) == NETDEV_ERROR);
    assert(length == 0U);

    netdev_stop(device);
    assert(stop_calls == 1);
    assert(!device->started && !device->link_up);
    assert(netdev_transmit(device, frame, 60U) == NETDEV_NOT_STARTED);
    puts("netdev tests passed");
    return 0;
}
