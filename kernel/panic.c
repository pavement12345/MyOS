#include "kernel.h"

_Noreturn void panic(const char *message) {
    serial_write("MyOS PANIC: ");
    serial_write(message != 0 ? message : "unspecified failure");
    serial_write("\n");
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
