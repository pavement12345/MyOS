#include "desktop.h"
#include "keyboard.h"

#define TITLE_CAPACITY 25U
#define TEXT_SCALE 2U

typedef struct {
    uint32_t background;
    uint32_t header;
    uint32_t panel;
    uint32_t border;
    uint32_t text;
    uint32_t muted;
    uint32_t accent;
} desktop_theme_t;

static const desktop_theme_t themes[] = {
    { 0x0B1020U, 0x111A2EU, 0x151F35U, 0x263552U, 0xE8EDF7U, 0x9BAAC3U, 0x8B7CFFU },
    { 0x081722U, 0x102331U, 0x142B3AU, 0x254556U, 0xE6F4FAU, 0x9DB9C8U, 0x39B6E8U },
    { 0x0C1A16U, 0x14271FU, 0x1A3026U, 0x315242U, 0xE8F4ECU, 0xA5C1AEU, 0x53D19AU },
    { 0x20110FU, 0x2D1915U, 0x38201AU, 0x59372EU, 0xF8EBE5U, 0xCDB1A7U, 0xFF9366U }
};

static const char *const theme_names[] = {
    "MIDNIGHT", "OCEAN", "FOREST", "EMBER"
};

static const uint8_t font[42][7] = {
    [1] = { 14, 17, 17, 31, 17, 17, 17 }, [2] = { 30, 17, 17, 30, 17, 17, 30 },
    [3] = { 14, 17, 16, 16, 16, 17, 14 }, [4] = { 30, 17, 17, 17, 17, 17, 30 },
    [5] = { 31, 16, 16, 30, 16, 16, 31 }, [6] = { 31, 16, 16, 30, 16, 16, 16 },
    [7] = { 14, 17, 16, 23, 17, 17, 15 }, [8] = { 17, 17, 17, 31, 17, 17, 17 },
    [9] = { 14, 4, 4, 4, 4, 4, 14 }, [10] = { 7, 2, 2, 2, 2, 18, 12 },
    [11] = { 17, 18, 20, 24, 20, 18, 17 }, [12] = { 16, 16, 16, 16, 16, 16, 31 },
    [13] = { 17, 27, 21, 21, 17, 17, 17 }, [14] = { 17, 25, 21, 19, 17, 17, 17 },
    [15] = { 14, 17, 17, 17, 17, 17, 14 }, [16] = { 30, 17, 17, 30, 16, 16, 16 },
    [17] = { 14, 17, 17, 17, 21, 18, 13 }, [18] = { 30, 17, 17, 30, 20, 18, 17 },
    [19] = { 15, 16, 16, 14, 1, 1, 30 }, [20] = { 31, 4, 4, 4, 4, 4, 4 },
    [21] = { 17, 17, 17, 17, 17, 17, 14 }, [22] = { 17, 17, 17, 17, 17, 10, 4 },
    [23] = { 17, 17, 17, 21, 21, 21, 10 }, [24] = { 17, 17, 10, 4, 10, 17, 17 },
    [25] = { 17, 17, 10, 4, 4, 4, 4 }, [26] = { 31, 1, 2, 4, 8, 16, 31 },
    [27] = { 14, 17, 19, 21, 25, 17, 14 }, [28] = { 4, 12, 4, 4, 4, 4, 14 },
    [29] = { 14, 17, 1, 2, 4, 8, 31 }, [30] = { 30, 1, 1, 14, 1, 1, 30 },
    [31] = { 2, 6, 10, 18, 31, 2, 2 }, [32] = { 31, 16, 16, 30, 1, 1, 30 },
    [33] = { 14, 16, 16, 30, 17, 17, 14 }, [34] = { 31, 1, 2, 4, 8, 8, 8 },
    [35] = { 14, 17, 17, 14, 17, 17, 14 }, [36] = { 14, 17, 17, 15, 1, 1, 14 },
    [37] = { 0, 4, 4, 0, 4, 4, 0 }, [38] = { 0, 0, 0, 31, 0, 0, 0 },
    [39] = { 0, 0, 0, 0, 0, 12, 12 }, [40] = { 1, 2, 2, 4, 8, 8, 16 },
    [41] = { 0, 0, 0, 0, 0, 0, 31 }
};

static char title[TITLE_CAPACITY] = "MYOS DESKTOP";
static char previous_title[TITLE_CAPACITY];
static uint32_t active_theme;
static bool editing_title;

static uint32_t glyph_index(char character) {
    if (character >= 'A' && character <= 'Z') {
        return 1U + (uint32_t)(character - 'A');
    }
    if (character >= '0' && character <= '9') {
        return 27U + (uint32_t)(character - '0');
    }
    switch (character) {
        case ':': return 37U;
        case '-': return 38U;
        case '.': return 39U;
        case '/': return 40U;
        case '_': return 41U;
        default: return 0U;
    }
}

static void draw_character(const framebuffer_t *framebuffer, uint32_t x, uint32_t y,
                           char character, uint32_t scale, uint32_t color) {
    if (character >= 'a' && character <= 'z') {
        character = (char)(character - 'a' + 'A');
    }
    uint32_t index = glyph_index(character);
    for (uint32_t row = 0U; row < 7U; ++row) {
        for (uint32_t column = 0U; column < 5U; ++column) {
            if ((font[index][row] & (1U << (4U - column))) != 0U) {
                framebuffer_fill_rect(framebuffer, x + column * scale, y + row * scale,
                                      scale, scale, color);
            }
        }
    }
}

static void draw_text(const framebuffer_t *framebuffer, uint32_t x, uint32_t y,
                      const char *text, uint32_t scale, uint32_t color) {
    while (*text != '\0' && x < framebuffer->width) {
        draw_character(framebuffer, x, y, *text++, scale, color);
        x += 6U * scale;
    }
}

static void draw_card(const framebuffer_t *framebuffer, const desktop_theme_t *theme,
                      uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    framebuffer_fill_rect(framebuffer, x, y, width, height, theme->border);
    if (width > 2U && height > 2U) {
        framebuffer_fill_rect(framebuffer, x + 1U, y + 1U, width - 2U, height - 2U,
                              theme->panel);
    }
}

static void draw_desktop(const framebuffer_t *framebuffer) {
    const desktop_theme_t *theme = &themes[active_theme];
    uint32_t width = framebuffer->width;
    uint32_t height = framebuffer->height;
    uint32_t main_x = 176U;
    uint32_t card_x = main_x;
    uint32_t card_width = width - card_x - 28U;
    uint32_t theme_y = height / 4U;
    uint32_t title_y = (height * 2U) / 3U;
    uint32_t title_height = height / 5U;

    framebuffer_clear(framebuffer, theme->background);
    framebuffer_fill_rect(framebuffer, 0U, 0U, width, 54U, theme->header);
    framebuffer_fill_rect(framebuffer, 0U, 54U, 150U, height - 54U, theme->header);
    framebuffer_fill_rect(framebuffer, 150U, 54U, 1U, height - 54U, theme->border);
    framebuffer_fill_rect(framebuffer, 18U, 17U, 20U, 20U, theme->accent);
    draw_text(framebuffer, 50U, 22U, title, TEXT_SCALE, theme->text);
    draw_text(framebuffer, width - 138U, 22U, "MYOS DESKTOP", 1U, theme->muted);

    draw_text(framebuffer, 22U, 82U, "WORKSPACE", 1U, theme->muted);
    framebuffer_fill_rect(framebuffer, 12U, 106U, 126U, 30U, theme->accent);
    draw_text(framebuffer, 25U, 116U, "APPEARANCE", 1U, theme->text);
    draw_text(framebuffer, 25U, 158U, "ABOUT", 1U, theme->muted);
    framebuffer_fill_rect(framebuffer, 18U, height - 42U, 12U, 12U, theme->accent);
    draw_text(framebuffer, 38U, height - 42U, theme_names[active_theme], 1U, theme->muted);

    draw_text(framebuffer, main_x, 78U, "PERSONALIZE", TEXT_SCALE, theme->text);
    draw_text(framebuffer, main_x, 100U, "SETTINGS FOR YOUR DESKTOP", 1U, theme->muted);

    draw_card(framebuffer, theme, card_x, theme_y, card_width, 174U);
    draw_text(framebuffer, card_x + 20U, theme_y + 18U, "COLOR THEME", TEXT_SCALE,
              theme->text);
    draw_text(framebuffer, card_x + 20U, theme_y + 46U, theme_names[active_theme], 1U,
              theme->accent);
    draw_text(framebuffer, card_x + 20U, theme_y + 72U, "PRESS T TO CYCLE THEMES", 1U,
              theme->muted);
    for (uint32_t i = 0U; i < (uint32_t)(sizeof(themes) / sizeof(themes[0])); ++i) {
        uint32_t swatch_x = card_x + 22U + i * 58U;
        uint32_t swatch_y = theme_y + 108U;
        if (i == active_theme) {
            framebuffer_fill_rect(framebuffer, swatch_x - 3U, swatch_y - 3U, 42U, 30U,
                                  theme->text);
        }
        framebuffer_fill_rect(framebuffer, swatch_x, swatch_y, 36U, 24U,
                              themes[i].accent);
    }

    draw_card(framebuffer, theme, card_x, title_y, card_width, title_height);
    draw_text(framebuffer, card_x + 20U, title_y + 16U, "DESKTOP TITLE", TEXT_SCALE,
              theme->text);
    framebuffer_fill_rect(framebuffer, card_x + 18U, title_y + 44U,
                          card_width - 36U, 34U, theme->header);
    if (title[0] != '\0') {
        draw_text(framebuffer, card_x + 28U, title_y + 54U, title, 1U, theme->text);
    } else {
        draw_text(framebuffer, card_x + 28U, title_y + 54U,
                  editing_title ? "TYPE A TITLE" : "NO TITLE", 1U, theme->muted);
    }

    if (editing_title) {
        draw_text(framebuffer, main_x, height - 32U, "TYPE TITLE / ENTER DONE / ESC CANCEL",
                  1U, theme->accent);
    } else {
        draw_text(framebuffer, main_x, height - 32U, "T CHANGE THEME   E EDIT TITLE",
                  1U, theme->muted);
    }
}

static bool title_character(uint8_t character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9') ||
           character == ' ' || character == '-' || character == '.' ||
           character == '/' || character == '_';
}

static void copy_title(char *destination, const char *source) {
    uint32_t index = 0U;
    while (index + 1U < TITLE_CAPACITY && source[index] != '\0') {
        destination[index] = source[index];
        ++index;
    }
    destination[index] = '\0';
}

_Noreturn void desktop_run(const framebuffer_t *framebuffer) {
    draw_desktop(framebuffer);

    for (;;) {
        uint8_t key;
        if (!keyboard_poll(&key)) {
            __asm__ volatile ("pause");
            continue;
        }

        if (editing_title) {
            if (key == KEYBOARD_KEY_ENTER) {
                editing_title = false;
            } else if (key == KEYBOARD_KEY_ESCAPE) {
                copy_title(title, previous_title);
                editing_title = false;
            } else if (key == KEYBOARD_KEY_BACKSPACE) {
                uint32_t length = 0U;
                while (length + 1U < TITLE_CAPACITY && title[length] != '\0') {
                    ++length;
                }
                if (length != 0U) {
                    title[length - 1U] = '\0';
                }
            } else if (title_character(key)) {
                uint32_t length = 0U;
                while (length + 1U < TITLE_CAPACITY && title[length] != '\0') {
                    ++length;
                }
                if (length + 1U < TITLE_CAPACITY) {
                    if (key >= 'a' && key <= 'z') {
                        key = (uint8_t)(key - 'a' + 'A');
                    }
                    title[length] = (char)key;
                    title[length + 1U] = '\0';
                }
            }
        } else if (key == 't' || key == 'T') {
            active_theme = (active_theme + 1U) %
                           (uint32_t)(sizeof(themes) / sizeof(themes[0]));
        } else if (key == 'e' || key == 'E') {
            copy_title(previous_title, title);
            title[0] = '\0';
            editing_title = true;
        }

        draw_desktop(framebuffer);
    }
}
