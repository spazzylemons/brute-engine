#ifndef BRUTE_I_INPUT_H
#define BRUTE_I_INPUT_H

/**
 * Functions for getting input.
 */

#include <stdbool.h>

// Button mask.
typedef enum {
    BTN_L = (1 << 0),
    BTN_R = (1 << 1),
    BTN_U = (1 << 2),
    BTN_D = (1 << 3),
    BTN_B = (1 << 4),
    BTN_A = (1 << 5),
    BTN_M = (1 << 6), // Menu "button".
} buttonmask_t;

// Get buttons held.
buttonmask_t I_GetHeldButtons(void);

// Get buttons pressed.
buttonmask_t I_GetPressedButtons(void);

#endif
