#ifndef MOUSE_H
#define MOUSE_H

#include "../types.h"

// Mouse state
typedef struct {
    int x, y;
    int buttons;
    int dx, dy;
} mouse_state_t;

// Function prototypes
void mouse_init(void);
uint8_t mouse_read(void);
mouse_state_t* mouse_get_state(void);
void mouse_interrupt_handler(void);

#endif // MOUSE_H