#include "mouse.h"
#include "../arch/interrupts.h"
#include "../net/eth.h"
#include "../drivers/vga.h"
#include "../gui.h" // For gui_process_mouse

// PS/2 mouse ports
#define MOUSE_DATA_PORT     0x60
#define MOUSE_STATUS_PORT   0x64
#define MOUSE_COMMAND_PORT  0x64

// Mouse commands
#define MOUSE_CMD_RESET     0xFF
#define MOUSE_CMD_RESEND    0xFE
#define MOUSE_CMD_SET_DEFAULTS 0xF6
#define MOUSE_CMD_DISABLE   0xF5
#define MOUSE_CMD_ENABLE    0xF4
#define MOUSE_CMD_SET_SAMPLE_RATE 0xF3
#define MOUSE_CMD_GET_DEVICE_ID 0xF2
#define MOUSE_CMD_SET_REMOTE_MODE 0xF0
#define MOUSE_CMD_SET_WRAP_MODE 0xEE
#define MOUSE_CMD_RESET_WRAP_MODE 0xEC
#define MOUSE_CMD_READ_DATA 0xEB
#define MOUSE_CMD_SET_STREAM_MODE 0xEA
#define MOUSE_CMD_STATUS_REQUEST 0xE9
#define MOUSE_CMD_SET_RESOLUTION 0xE8
#define MOUSE_CMD_SET_SCALING_2_1 0xE7
#define MOUSE_CMD_SET_SCALING_1_1 0xE6

// Global mouse state
static mouse_state_t mouse_state;
static uint8_t mouse_cycle = 0;
static int8_t mouse_bytes[3];

// Wait for mouse to be ready to send
static void mouse_wait_write(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if ((inb(MOUSE_STATUS_PORT) & 0x02) == 0) {
            return;
        }
    }
}

// Wait for mouse to have data
static void mouse_wait_read(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if (inb(MOUSE_STATUS_PORT) & 0x01) {
            return;
        }
    }
}

// Send command to mouse
static void mouse_write(uint8_t data) {
    mouse_wait_write();
    outb(MOUSE_COMMAND_PORT, 0xD4);
    mouse_wait_write();
    outb(MOUSE_DATA_PORT, data);
}

// Read from mouse
uint8_t mouse_read(void) {
    mouse_wait_read();
    return inb(MOUSE_DATA_PORT);
}

// Initialize mouse
void mouse_init(void) {
    uint8_t status;

    // Enable auxiliary device
    mouse_wait_write();
    outb(MOUSE_COMMAND_PORT, 0xA8);

    // Enable interrupts
    mouse_wait_write();
    outb(MOUSE_COMMAND_PORT, 0x20);
    mouse_wait_read();
    status = inb(MOUSE_DATA_PORT) | 0x02;
    mouse_wait_write();
    outb(MOUSE_COMMAND_PORT, 0x60);
    mouse_wait_write();
    outb(MOUSE_DATA_PORT, status);

    // Set defaults
    mouse_write(MOUSE_CMD_SET_DEFAULTS);
    mouse_read(); // ACK

    // Enable data reporting
    mouse_write(MOUSE_CMD_ENABLE);
    mouse_read(); // ACK

    // Reset mouse state
    mouse_state.x = 320 / 2; // Center
    mouse_state.y = 200 / 2;
    mouse_state.buttons = 0;
    mouse_state.dx = 0;
    mouse_state.dy = 0;

    vga_write_string("Mouse initialized\n");
}

// Handle mouse interrupt
void mouse_interrupt_handler(void) {
    uint8_t data = inb(MOUSE_DATA_PORT);

    switch (mouse_cycle) {
        case 0:
            if ((data & 0x08) == 0) break; // Bit 3 must be set
            mouse_bytes[0] = data;
            mouse_cycle++;
            break;
        case 1:
            mouse_bytes[1] = data;
            mouse_cycle++;
            break;
        case 2:
            mouse_bytes[2] = data;
            mouse_cycle = 0;

            // Update mouse state
            mouse_state.buttons = mouse_bytes[0] & 0x07;
            mouse_state.dx = mouse_bytes[1];
            mouse_state.dy = mouse_bytes[2];

            // Update position (simple, no acceleration)
            mouse_state.x += mouse_state.dx;
            mouse_state.y -= mouse_state.dy; // Y is inverted

            // Clamp to screen
            if (mouse_state.x < 0) mouse_state.x = 0;
            if (mouse_state.x >= 320) mouse_state.x = 319;
            if (mouse_state.y < 0) mouse_state.y = 0;
            if (mouse_state.y >= 200) mouse_state.y = 199;

            // Notify GUI of mouse update
            gui_process_mouse(mouse_state.x, mouse_state.y, mouse_state.buttons);

            break;
    }

    // Send EOI
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}

// Get current mouse state
mouse_state_t* mouse_get_state(void) {
    return &mouse_state;
}