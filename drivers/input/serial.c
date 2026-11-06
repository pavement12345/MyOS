#include "serial.h"

#define COM1_DATA 0x3F8U
#define COM1_IER  0x3F9U
#define COM1_FCR  0x3FAU
#define COM1_LCR  0x3FBU
#define COM1_MCR  0x3FCU
#define COM1_LSR  0x3FDU

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_putc(char ch) {
    while ((inb(COM1_LSR) & 0x20U) == 0U) {
        __asm__ volatile ("pause");
    }
    outb(COM1_DATA, (uint8_t)ch);
}

void serial_initialize(void) {
    outb(COM1_IER, 0x00); /* Polled operation: no UART interrupts. */
    outb(COM1_LCR, 0x80); /* Enable divisor latch. */
    outb(COM1_DATA, 0x01); /* Divisor 1: 115200 baud. */
    outb(COM1_IER, 0x00);
    outb(COM1_LCR, 0x03); /* 8 data bits, no parity, one stop bit. */
    outb(COM1_FCR, 0xC7); /* Enable and clear FIFO. */
    outb(COM1_MCR, 0x0B); /* DTR, RTS, and OUT2. */
}

void serial_write(const char *text) {
    if (text == 0) {
        return;
    }
    while (*text != '\0') {
        if (*text == '\n') {
            serial_putc('\r');
        }
        serial_putc(*text++);
    }
}

char serial_read_char(void) {
    while ((inb(COM1_LSR) & 0x01U) == 0U) {
        __asm__ volatile ("pause");
    }
    return (char)inb(COM1_DATA);
}
