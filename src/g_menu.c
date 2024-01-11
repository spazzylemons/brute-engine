#include "g_font.h"
#include "g_menu.h"
#include "i_input.h"
#include "i_video.h"
#include "r_draw.h"
#include "w_pack.h"
#include "z_memory.h"

// X offset to draw menu text.
#define MENUXOFF 120
// X offset to draw choice text.
#define MENUCHOICEX 240
// Y offset to draw menu text.
#define MENUYOFF 50
// Spacing between menu items.
#define MENUYSPC 20

#define ARRLEN(_arr_) sizeof(_arr_) / sizeof((_arr_)[0])

// Temporary byte for detail level.
static uint8_t detaillevel = 0;

// Menu font.
static font_t *font;

// Menu pointer icon.
static bob_t *pointer;

typedef enum {
    // Widget that displays an enumerated value from a list of options.
    WIDGET_CHOICE,
} widgettype_t;

// Choice widget.
typedef struct {
    // Pointer to byte containing selected choice.
    uint8_t *ref;
    uint8_t numchoices;
    // List of choices.
    const char *const *choices;
} widgetchoice_t;

// Menu widget.
typedef struct {
    widgettype_t type;
    union {
        widgetchoice_t choice;
    };
} menuwidget_t;

// Menu item.
typedef struct {
    // Text to draw.
    const char *label;
    // If item.
    const struct menu_s *target;
    // Optional widget to draw with the model.
    const menuwidget_t *widget;
} menuitem_t;

// Menu.
typedef struct menu_s {
    struct menu_s    *parent;   // Menu to return to. If NULL, this is the root.
    uint8_t           numitems; // Number of menu items.
    const menuitem_t *items;    // Menu items.
} menu_t;

// Include generated menu.
#include "g_menugen.inl"

// Current menu.
static const menu_t *currentmenu = NULL;
// Current menu index.
static uint8_t itemindex;

static void SelectMenu(const menu_t *menu) {
    currentmenu = menu;
    itemindex = 0;
}

static void PrevChoice(const widgetchoice_t *choice) {
    if (*choice->ref == 0) {
        *choice->ref = choice->numchoices - 1;
    } else {
        --*choice->ref;
    }
}

static void NextChoice(const widgetchoice_t *choice) {
    if (*choice->ref == choice->numchoices - 1) {
        *choice->ref = 0;
    } else {
        ++*choice->ref;
    }
}

// Dimming blit pattern.
static uint8_t dimblit[208];

void G_MenuInit(void) {
    uint32_t bobs = W_GetNumByName(ROOTID, "bobs");
    uint32_t fonts = W_GetNumByName(ROOTID, "fonts");

    font = G_LoadFont(W_GetNumByName(fonts, "menu"));
    pointer = G_LoadBob(W_GetNumByName(bobs, "pointer"));

    // Generate dimblit.
    uint8_t i = 0;
    while (i < 104) {
        dimblit[i++] = 0x55;
        dimblit[i++] = 0x00;
    }
    while (i < 208) {
        dimblit[i++] = 0xaa;
        dimblit[i++] = 0x00;
    }
}

void G_MenuDeinit(void) {
    G_FreeFont(font);
    Deallocate(pointer);
}

static void DimBackground(void) {
    blit_source = dimblit;
    blit_length = 104;
    blit_x = 0;
    for (uint8_t y = 0; y < 240; y += 2) {
        blit_y = y;
        R_Blit();
    }
}

static const menuitem_t *CurrentItem(void) {
    return &currentmenu->items[itemindex];
}

void G_MenuReset(void) {
    SelectMenu(&menu_main);
}

static void HandleWidget(buttonmask_t pressed) {
    const menuwidget_t *widget = CurrentItem()->widget;
    if (widget != NULL) {
        switch (widget->type) {
            case WIDGET_CHOICE: {
                if (pressed & (BTN_R | BTN_A)) {
                    NextChoice(&widget->choice);
                } else if (pressed & BTN_L) {
                    PrevChoice(&widget->choice);
                }
                break;
            }
        }
    }
}

void G_MenuShow(void) {
    // Check input.
    buttonmask_t pressed = I_GetPressedButtons();
    // If B pressed, go to parent menu.
    if (pressed & BTN_B) {
        SelectMenu(currentmenu->parent);
        // If menu is now NULL, quit.
        if (currentmenu == NULL) {
            return;
        }
    }
    // If menu has a target, switch to that target.
    if (pressed & BTN_A) {
        if (CurrentItem()->target != NULL) {
            SelectMenu(CurrentItem()->target);
        }
    }
    // Handle widget interactions.
    HandleWidget(pressed);
    // If up or down pressed, move pointer accordingly.
    if (pressed & BTN_U) {
        if (itemindex == 0) {
            itemindex = currentmenu->numitems - 1;
        } else {
            --itemindex;
        }
    } else if (pressed & BTN_D) {
        if (itemindex == currentmenu->numitems - 1) {
            itemindex = 0;
        } else {
            ++itemindex;
        }
    }
    // Dim background to make menu clearer.
    DimBackground();
    // Draw current menu items.
    uint8_t y = MENUYOFF;
    for (uint8_t i = 0; i < currentmenu->numitems; i++) {
        // Get the menu item.
        const menuitem_t *item = &currentmenu->items[i];
        // If this is the item index, draw the pointer.
        if (i == itemindex) {
            G_DrawBob(pointer, MENUXOFF - 20, y);
        }
        // Draw the menu text.
        G_PrintFont(font, item->label, MENUXOFF, y);
        // If there is a widget, draw it.
        if (item->widget != NULL) {
            switch (item->widget->type) {
                case WIDGET_CHOICE: {
                    // Index the choice strings and print the choice.
                    uint8_t choice = *item->widget->choice.ref;
                    G_PrintFont(font, item->widget->choice.choices[choice], MENUCHOICEX, y);
                    break;
                }
            }
        }
        y += MENUYSPC;
    }
}

bool G_IsMenuOpen(void) {
    return currentmenu != NULL;
}
