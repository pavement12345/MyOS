#ifndef MYOS_DESKTOP_H
#define MYOS_DESKTOP_H

#include "framebuffer.h"

_Noreturn void desktop_run(const framebuffer_t *framebuffer, bool mouse_available);

#endif
