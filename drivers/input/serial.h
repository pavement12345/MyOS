#ifndef MYOS_SERIAL_H
#define MYOS_SERIAL_H

#include <stdint.h>

void serial_initialize(void);
void serial_write(const char *text);
char serial_read_char(void);

#endif
