#include "commands.h"
#include "serial.h"

static int string_equal(const char *left, const char *right) {
    while (*left != '\0' && *right != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

static int is_space(char ch) {
    return ch == ' ' || ch == '\t';
}

static void print_help(void) {
    serial_write("Available commands:\n");
    serial_write("  help        Show this command list\n");
    serial_write("  clear       Clear an ANSI-compatible terminal\n");
    serial_write("  echo TEXT   Print TEXT\n");
    serial_write("  version     Show MyOS version\n");
    serial_write("  systeminfo  Show implemented system details\n");
    serial_write("  halt        Halt the CPU\n");
}

static void print_system_info(void) {
    serial_write("MyOS x86_64 development kernel\n");
    serial_write("Boot: GRUB Multiboot2 BIOS path\n");
    serial_write("Console: polled COM1 UART\n");
    serial_write("Available: kernel entry and serial shell\n");
    serial_write("Unavailable: filesystem, processes, graphics, networking\n");
}

void shell_execute(char *line) {
    char *command = line;
    while (is_space(*command)) {
        ++command;
    }
    if (*command == '\0') {
        return;
    }

    char *arguments = command;
    while (*arguments != '\0' && !is_space(*arguments)) {
        ++arguments;
    }
    if (*arguments != '\0') {
        *arguments++ = '\0';
        while (is_space(*arguments)) {
            ++arguments;
        }
    }

    if (string_equal(command, "help")) {
        print_help();
    } else if (string_equal(command, "clear")) {
        serial_write("\033[2J\033[H");
    } else if (string_equal(command, "echo")) {
        serial_write(arguments);
        serial_write("\n");
    } else if (string_equal(command, "version")) {
        serial_write("MyOS development kernel 0.1\n");
    } else if (string_equal(command, "systeminfo")) {
        print_system_info();
    } else if (string_equal(command, "halt")) {
        serial_write("CPU halted. Reset the virtual machine to continue.\n");
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    } else {
        serial_write("Unknown command: ");
        serial_write(command);
        serial_write(" (type 'help')\n");
    }
}
