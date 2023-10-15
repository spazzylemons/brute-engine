#include "b_core.h"
#include "u_error.h"
#include "u_format.h"

#include <stdbool.h>
#include <stdint.h>

static char errorbuf[256];

// The jump buffer.
#ifdef __ARM_ARCH
uint32_t jumpbuffer[10];
#else
static uint64_t jumpbuffer[8];
#endif

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


__attribute__((noinline))
noreturn void Error(const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    FormatStringV(errorbuf, sizeof(errorbuf), fmt, list);
    va_end(list);

#ifdef TARGET_PLAYDATE
    asm volatile (
        ".global jumpbuffer\n"
        "ldr   r0, =jumpbuffer\n"
        "ldm   r0!, {r2, r4, r5, r6, r7, r8, r9, sl, fp, lr}\n"
        "mov   sp, r2\n"
        "mov   r0, #1\n"
        "bx    lr\n"
    );
#else
    asm volatile (
        "lea jumpbuffer(%rip), %rdi\n"
        "mov $1, %eax\n"
        "mov (%rdi), %rbx\n"
        "mov 8(%rdi), %rbp\n"
        "mov 16(%rdi), %r12\n"
        "mov 24(%rdi), %r13\n"
        "mov 32(%rdi), %r14\n"
        "mov 40(%rdi), %r15\n"
        "mov 48(%rdi), %rsp\n"
        "jmp *56(%rdi)\n"
    );
#endif
    __builtin_unreachable();
}

#pragma GCC push_options
#pragma GCC optimize "-fomit-frame-pointer"
__attribute__((noinline))
int CatchError(void) {
#ifdef TARGET_PLAYDATE
    asm volatile (
        ".global jumpbuffer\n"
        "ldr   r0, =jumpbuffer\n"
        "mov   r2, sp\n"
        "stmia r0!, {r2, r4, r5, r6, r7, r8, r9, sl, fp, lr}\n"
        "mov   r0, #0\n"
        "bx    lr\n"
    );
#else
    asm volatile (
        "lea jumpbuffer(%rip), %rdi\n"
        "mov %rbx, (%rdi)\n"
        "mov %rbp, 8(%rdi)\n"
        "mov %r12, 16(%rdi)\n"
        "mov %r13, 24(%rdi)\n"
        "mov %r14, 32(%rdi)\n"
        "mov %r15, 40(%rdi)\n"
        "lea 8(%rsp), %rdx\n"
        "mov %rdx, 48(%rdi)\n"
        "mov (%rsp), %rdx\n"
        "mov %rdx, 56(%rdi)\n"
        "xor %eax, %eax\n"
        "ret\n"
    );
#endif
    __builtin_unreachable();
}
#pragma GCC pop_options
