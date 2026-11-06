#include "shell.h"
#include "commands.h"
#include "serial.h"

#define SHELL_LINE_CAPACITY 128U

static void shell_prompt(void) {
    serial_write("myos> ");
}

static void shell_read_line(char *line, unsigned capacity) {
    unsigned length = 0;
    shell_prompt();

    for (;;) {
        char ch = serial_read_char();
        if (ch == '\r' || ch == '\n') {
            serial_write("\n");
            line[length] = '\0';
            return;
        }
        if (ch == '\b' || ch == 0x7F) {
            if (length != 0U) {
                --length;
                serial_write("\b \b");
            }
            continue;
        }
        if ((unsigned char)ch < 0x20U) {
            continue;
        }
        if (length + 1U >= capacity) {
            serial_write("\nInput too long; line discarded.\n");
            do {
                ch = serial_read_char();
            } while (ch != '\r' && ch != '\n');
            line[0] = '\0';
            return;
        }
        line[length++] = ch;
        char echo[2] = { ch, '\0' };
        serial_write(echo);
    }
}

_Noreturn void shell_run(void) {
    char line[SHELL_LINE_CAPACITY];
    serial_write("MyShell: polled COM1 console ready. Type 'help' for commands.\n");
    for (;;) {
        shell_read_line(line, SHELL_LINE_CAPACITY);
        if (line[0] != '\0') {
            shell_execute(line);
        }
    }
}
