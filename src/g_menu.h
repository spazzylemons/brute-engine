#ifndef BRUTE_G_MENU_H
#define BRUTE_G_MENU_H

/**
 * Pause menu.
 */

#include <stdbool.h>

void G_MenuInit(void);

void G_MenuDeinit(void);

void G_MenuShow(void);

void G_MenuReset(void);

bool G_IsMenuOpen(void);

#endif
