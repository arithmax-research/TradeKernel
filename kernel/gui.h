#ifndef GUI_H
#define GUI_H

#include "types.h"
#include "drivers/vga.h"

// GUI constants
#define GUI_MAX_WINDOWS 8
#define GUI_MAX_WIDGETS 32
#define GUI_TITLE_BAR_HEIGHT 1
#define GUI_BORDER_WIDTH 1
#define GUI_TERMINAL_BUFFER_SIZE 1024
#define GUI_TERMINAL_WIDTH 68
#define GUI_TERMINAL_HEIGHT 18

// GUI colors
#define GUI_COLOR_TITLE_BAR_FG VGA_COLOR_WHITE
#define GUI_COLOR_TITLE_BAR_BG VGA_COLOR_BLUE
#define GUI_COLOR_WINDOW_FG VGA_COLOR_LIGHT_GREY
#define GUI_COLOR_WINDOW_BG VGA_COLOR_BLACK
#define GUI_COLOR_BORDER_FG VGA_COLOR_LIGHT_CYAN
#define GUI_COLOR_BUTTON_FG VGA_COLOR_BLACK
#define GUI_COLOR_BUTTON_BG VGA_COLOR_LIGHT_GREY
#define GUI_COLOR_BUTTON_ACTIVE_FG VGA_COLOR_WHITE
#define GUI_COLOR_BUTTON_ACTIVE_BG VGA_COLOR_BLUE

// Widget types
typedef enum {
    WIDGET_BUTTON,
    WIDGET_LABEL,
    WIDGET_TEXTBOX,
    WIDGET_CHECKBOX
} widget_type_t;

// Widget structure
typedef struct widget {
    widget_type_t type;
    int x, y;           // Position relative to window
    int width, height;
    char* text;
    int active;         // For buttons: pressed state, for checkboxes: checked state
    void (*callback)(struct widget*); // Callback function
    struct widget* next;
} widget_t;

// Window structure
typedef struct window {
    int id;
    int x, y;           // Screen position
    int width, height;
    char title[32];
    int visible;
    int focused;
    int is_terminal;    // Is this a terminal window?
    char terminal_buffer[GUI_TERMINAL_BUFFER_SIZE]; // Terminal output buffer
    int terminal_pos;   // Current position in buffer
    int terminal_scroll; // Scroll position for display
    widget_t* widgets;  // Linked list of widgets
    struct window* next;
} window_t;

// GUI state
typedef struct {
    window_t* windows;
    window_t* focused_window;
    int next_window_id;
} gui_state_t;

// Function prototypes
void gui_init(void);
window_t* gui_create_window(int x, int y, int width, int height, const char* title);
window_t* gui_create_terminal_window(int x, int y, int width, int height, const char* title);
void gui_destroy_window(window_t* window);
void gui_show_window(window_t* window);
void gui_hide_window(window_t* window);
void gui_focus_window(window_t* window);

void gui_terminal_write(window_t* window, const char* str);
void gui_terminal_putchar(window_t* window, char c);
void gui_terminal_clear(window_t* window);
void gui_terminal_search(window_t* window, const char* query);

widget_t* gui_create_button(window_t* window, int x, int y, int width, int height, const char* text, void (*callback)(widget_t*));
widget_t* gui_create_label(window_t* window, int x, int y, const char* text);
widget_t* gui_create_checkbox(window_t* window, int x, int y, const char* text, int checked);

void gui_draw_window(window_t* window);
void gui_draw_widget(widget_t* widget, window_t* window);
void gui_redraw_all(void);

void gui_handle_input(char c);
void gui_process_mouse(int x, int y, int button);
void gui_draw_cursor(void);

#endif // GUI_H