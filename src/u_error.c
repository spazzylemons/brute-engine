#include "b_core.h"
#include "u_error.h"
#include "u_format.h"

#include <stdbool.h>
#include <stdint.h>

static char errorbuf[256];

intptr_t jumpbuffer[5];

static void DrawCenteredMessage(int y, const char *message) {
    size_t size = strlen(message);
    int width = playdate->graphics->getTextWidth(NULL, message, size, kUTF8Encoding, 0);
    int x = (playdate->display->getWidth() - width) >> 1;
    playdate->graphics->drawText(message, size, kUTF8Encoding, x, y);
}

void DisplayError(void) {
    // Clear screen to black.
    playdate->graphics->clear(kColorBlack);
    // Use system font.
    playdate->graphics->setFont(NULL);
    // Use white color for text.
    playdate->graphics->setDrawMode(kDrawModeFillWhite);
    // Draw centered error text.
    DrawCenteredMessage(50, "An error occurred:");
    DrawCenteredMessage(70, errorbuf);
    DrawCenteredMessage(90, "The game will not continue.");
    // Make sure this shows up.
    playdate->graphics->display();
}

noreturn void Error(const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    FormatStringV(errorbuf, sizeof(errorbuf), fmt, list);
    va_end(list);

    // Log the message in the console as well, in case the application
    // terminates before we can see it.
    playdate->system->logToConsole("%s", errorbuf);

    __builtin_longjmp(jumpbuffer, 1);
}
