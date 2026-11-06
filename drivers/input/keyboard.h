#ifndef MYOS_KEYBOARD_H
#define MYOS_KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>

#define KEYBOARD_KEY_ENTER 0x80U
#define KEYBOARD_KEY_BACKSPACE 0x81U
#define KEYBOARD_KEY_ESCAPE 0x82U

bool keyboard_poll(uint8_t *key);

#endif
