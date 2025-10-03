#include "interrupts.h"
#include "../drivers/vga.h"

#define IDT_SIZE 256
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

static idt_entry_t idt[IDT_SIZE];
static idt_descriptor_t idt_desc;

// I/O port access functions
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void set_idt_entry(int num, uint32_t handler, uint16_t selector, uint8_t flags) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_high = (handler >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

// Initialize the PIC (Programmable Interrupt Controller)
static void init_pic(void) {
    // Initialize PIC1
    outb(PIC1_COMMAND, 0x11); // Start initialization
    outb(PIC1_DATA, 0x20);    // IRQ 0-7 mapped to interrupts 0x20-0x27
    outb(PIC1_DATA, 0x04);    // PIC1 is master
    outb(PIC1_DATA, 0x01);    // 8086 mode
    
    // Initialize PIC2
    outb(PIC2_COMMAND, 0x11); // Start initialization
    outb(PIC2_DATA, 0x28);    // IRQ 8-15 mapped to interrupts 0x28-0x2F
    outb(PIC2_DATA, 0x02);    // PIC2 is slave
    outb(PIC2_DATA, 0x01);    // 8086 mode
    
    // Enable keyboard and timer interrupts only
    outb(PIC1_DATA, 0xFC); // Enable IRQ 0 (timer) and IRQ 1 (keyboard)
    outb(PIC2_DATA, 0xFF); // Disable all IRQs on PIC2
}

void interrupts_init(void) {
    // Set up IDT descriptor
    idt_desc.limit = sizeof(idt) - 1;
    idt_desc.base = (uint32_t)&idt;
    
    // Clear IDT
    for (int i = 0; i < IDT_SIZE; i++) {
        set_idt_entry(i, 0, 0, 0);
    }
    
    // Set interrupt handlers
    set_idt_entry(0x20, (uint32_t)timer_interrupt_wrapper, 0x08, 0x8E); // Timer
    set_idt_entry(0x21, (uint32_t)keyboard_interrupt_wrapper, 0x08, 0x8E); // Keyboard
    
    // Initialize PIC
    init_pic();
    
    // Load IDT
    __asm__ volatile ("lidt %0" : : "m"(idt_desc));
    
    // Enable interrupts
    __asm__ volatile ("sti");
}

// Timer interrupt handler
void timer_handler(void) {
    static int tick_count = 0;
    tick_count++;
    
    // Send End of Interrupt to PIC
    outb(PIC1_COMMAND, 0x20);
}

// Keyboard interrupt handler
void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    
    // Simple scancode to ASCII mapping (only for basic keys)
    static const char scancode_to_ascii[] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' '
    };
    
    // Only handle key press events (not key release)
    if (!(scancode & 0x80) && scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];
        if (c != 0) {
            vga_putchar(c);
        }
    }
    
    // Send End of Interrupt to PIC
    outb(PIC1_COMMAND, 0x20);
}