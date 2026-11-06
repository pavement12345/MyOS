#ifndef MYOS_MOUSE_H
#define MYOS_MOUSE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t x;
    uint32_t y;
    uint8_t buttons;
} mouse_state_t;

bool mouse_initialize(uint32_t screen_width, uint32_t screen_height);
bool mouse_poll(mouse_state_t *state);

#endif
