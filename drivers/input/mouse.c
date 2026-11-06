#include "mouse.h"

#define PS2_STATUS_PORT 0x64U
#define PS2_COMMAND_PORT 0x64U
#define PS2_DATA_PORT 0x60U
#define PS2_AUX_OUTPUT 0x20U
#define PS2_OUTPUT_FULL 0x01U
#define PS2_INPUT_FULL 0x02U
#define PS2_WAIT_LIMIT 100000U

static mouse_state_t current_state;
static uint32_t max_x;
static uint32_t max_y;
static uint8_t packet[3];
static uint8_t packet_bytes;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static bool wait_input_empty(void) {
    for (uint32_t attempt = 0U; attempt < PS2_WAIT_LIMIT; ++attempt) {
        if ((inb(PS2_STATUS_PORT) & PS2_INPUT_FULL) == 0U) {
            return true;
        }
        __asm__ volatile ("pause");
    }
    return false;
}

static bool wait_output_full(bool auxiliary) {
    for (uint32_t attempt = 0U; attempt < PS2_WAIT_LIMIT; ++attempt) {
        uint8_t status = inb(PS2_STATUS_PORT);
        if ((status & PS2_OUTPUT_FULL) != 0U &&
            (((status & PS2_AUX_OUTPUT) != 0U) == auxiliary)) {
            return true;
        }
        __asm__ volatile ("pause");
    }
    return false;
}

static bool controller_command(uint8_t command) {
    if (!wait_input_empty()) {
        return false;
    }
    outb(PS2_COMMAND_PORT, command);
    return true;
}

static bool read_command_byte(uint8_t *command_byte) {
    if (!controller_command(0x20U) || !wait_output_full(false)) {
        return false;
    }
    *command_byte = inb(PS2_DATA_PORT);
    return true;
}

static bool write_command_byte(uint8_t command_byte) {
    if (!controller_command(0x60U) || !wait_input_empty()) {
        return false;
    }
    outb(PS2_DATA_PORT, command_byte);
    return wait_input_empty();
}

static bool write_mouse(uint8_t value) {
    if (!controller_command(0xD4U) || !wait_input_empty()) {
        return false;
    }
    outb(PS2_DATA_PORT, value);
    if (!wait_output_full(true)) {
        return false;
    }
    return inb(PS2_DATA_PORT) == 0xFAU;
}

bool mouse_initialize(uint32_t screen_width, uint32_t screen_height) {
    if (screen_width == 0U || screen_height == 0U) {
        return false;
    }

    for (uint32_t attempt = 0U; attempt < PS2_WAIT_LIMIT; ++attempt) {
        if ((inb(PS2_STATUS_PORT) & PS2_OUTPUT_FULL) == 0U) {
            break;
        }
        (void)inb(PS2_DATA_PORT);
    }

    if (!controller_command(0xA8U)) {
        return false;
    }

    uint8_t command_byte;
    if (!read_command_byte(&command_byte)) {
        return false;
    }
    command_byte = (uint8_t)((command_byte | 0x20U) & (uint8_t)~0x02U);
    if (!write_command_byte((uint8_t)(command_byte & (uint8_t)~0x20U)) ||
        !write_mouse(0xF6U) || !write_mouse(0xF4U)) {
        return false;
    }

    max_x = screen_width;
    max_y = screen_height;
    current_state.x = screen_width / 2U;
    current_state.y = screen_height / 2U;
    current_state.buttons = 0U;
    packet_bytes = 0U;
    return true;
}

bool mouse_poll(mouse_state_t *state) {
    if (state == 0) {
        return false;
    }

    uint8_t status = inb(PS2_STATUS_PORT);
    if ((status & PS2_OUTPUT_FULL) == 0U || (status & PS2_AUX_OUTPUT) == 0U) {
        return false;
    }

    uint8_t value = inb(PS2_DATA_PORT);
    if (packet_bytes == 0U && (value & 0x08U) == 0U) {
        return false;
    }

    packet[packet_bytes++] = value;
    if (packet_bytes < 3U) {
        return false;
    }
    packet_bytes = 0U;

    if ((packet[0] & 0xC0U) == 0U) {
        int32_t delta_x = (int32_t)packet[1] - ((packet[0] & 0x10U) != 0U ? 256 : 0);
        int32_t delta_y = (int32_t)packet[2] - ((packet[0] & 0x20U) != 0U ? 256 : 0);
        int32_t x = (int32_t)current_state.x + delta_x;
        int32_t y = (int32_t)current_state.y - delta_y;

        if (x < 0) {
            x = 0;
        }
        if (y < 0) {
            y = 0;
        }

        if ((uint32_t)x >= max_x) {
            x = (int32_t)(max_x - 1U);
        }
        if ((uint32_t)y >= max_y) {
            y = (int32_t)(max_y - 1U);
        }
        current_state.x = (uint32_t)x;
        current_state.y = (uint32_t)y;
    }
    current_state.buttons = packet[0] & 0x07U;
    *state = current_state;
    return true;
}
