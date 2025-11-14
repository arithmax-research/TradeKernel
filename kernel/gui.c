#include "gui.h"
#include "mm/memory.h"
#include "drivers/mouse.h" // For mouse state

// Global GUI state
static gui_state_t gui_state;
static int cursor_x = 160; // Center
static int cursor_y = 100;
static int drag_window = 0; // Is a window being dragged?
static int drag_offset_x = 0; // Offset from cursor to window origin
static int drag_offset_y = 0;

// Initialize GUI system
void gui_init(void) {
    memset(&gui_state, 0, sizeof(gui_state_t));
    gui_state.next_window_id = 1;
    
    // Switch to graphics mode for GUI
    // vga_set_graphics_mode(); // Comment out for now
    // vga_clear(); // Clear to black
}

// Create a new window
window_t* gui_create_window(int x, int y, int width, int height, const char* title) {
    window_t* window = (window_t*)kmalloc(sizeof(window_t));
    if (!window) return NULL;
    
    window->id = gui_state.next_window_id++;
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    strncpy(window->title, title, sizeof(window->title) - 1);
    window->title[sizeof(window->title) - 1] = '\0';
    window->visible = 0;
    window->focused = 0;
    window->is_terminal = 0;
    window->terminal_pos = 0;
    window->terminal_scroll = 0;
    memset(window->terminal_buffer, 0, GUI_TERMINAL_BUFFER_SIZE);
    window->widgets = NULL;
    window->next = gui_state.windows;
    
    gui_state.windows = window;
    return window;
}

// Create a terminal window
window_t* gui_create_terminal_window(int x, int y, int width, int height, const char* title) {
    window_t* window = gui_create_window(x, y, width, height, title);
    if (window) {
        window->is_terminal = 1;
        gui_terminal_write(window, "$ ");
    }
    return window;
}

// Destroy a window and all its widgets
void gui_destroy_window(window_t* window) {
    if (!window) return;
    
    // Remove from window list
    if (gui_state.windows == window) {
        gui_state.windows = window->next;
    } else {
        window_t* prev = gui_state.windows;
        while (prev && prev->next != window) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = window->next;
        }
    }
    
    // Destroy all widgets
    widget_t* widget = window->widgets;
    while (widget) {
        widget_t* next = widget->next;
        if (widget->text) kfree(widget->text);
        kfree(widget);
        widget = next;
    }
    
    kfree(window);
}

// Show a window
void gui_show_window(window_t* window) {
    if (!window) return;
    window->visible = 1;
    gui_focus_window(window);
    gui_draw_window(window);
}

// Hide a window
void gui_hide_window(window_t* window) {
    if (!window) return;
    window->visible = 0;
    gui_redraw_all();
}

// Focus a window
void gui_focus_window(window_t* window) {
    if (!window) return;
    
    // Unfocus all windows
    window_t* w = gui_state.windows;
    while (w) {
        w->focused = 0;
        w = w->next;
    }
    
    // Focus the specified window
    window->focused = 1;
    gui_state.focused_window = window;
    
    // Move window to front (end of list)
    if (gui_state.windows != window) {
        // Remove from current position
        window_t* prev = gui_state.windows;
        while (prev && prev->next != window) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = window->next;
        }
        
        // Add to end
        window->next = NULL;
        w = gui_state.windows;
        if (w) {
            while (w->next) w = w->next;
            w->next = window;
        } else {
            gui_state.windows = window;
        }
    }
    
    gui_redraw_all();
}

// Create a button widget
widget_t* gui_create_button(window_t* window, int x, int y, int width, int height, const char* text, void (*callback)(widget_t*)) {
    if (!window) return NULL;
    
    widget_t* widget = (widget_t*)kmalloc(sizeof(widget_t));
    if (!widget) return NULL;
    
    widget->type = WIDGET_BUTTON;
    widget->x = x;
    widget->y = y;
    widget->width = width;
    widget->height = height;
    widget->text = kmalloc(strlen(text) + 1);
    if (widget->text) {
        strcpy(widget->text, text);
    }
    widget->active = 0;
    widget->callback = callback;
    widget->next = window->widgets;
    
    window->widgets = widget;
    return widget;
}

// Create a label widget
widget_t* gui_create_label(window_t* window, int x, int y, const char* text) {
    if (!window) return NULL;
    
    widget_t* widget = (widget_t*)kmalloc(sizeof(widget_t));
    if (!widget) return NULL;
    
    widget->type = WIDGET_LABEL;
    widget->x = x;
    widget->y = y;
    widget->width = strlen(text);
    widget->height = 1;
    widget->text = kmalloc(strlen(text) + 1);
    if (widget->text) {
        strcpy(widget->text, text);
    }
    widget->active = 0;
    widget->callback = NULL;
    widget->next = window->widgets;
    
    window->widgets = widget;
    return widget;
}

// Create a checkbox widget
widget_t* gui_create_checkbox(window_t* window, int x, int y, const char* text, int checked) {
    if (!window) return NULL;
    
    widget_t* widget = (widget_t*)kmalloc(sizeof(widget_t));
    if (!widget) return NULL;
    
    widget->type = WIDGET_CHECKBOX;
    widget->x = x;
    widget->y = y;
    widget->width = strlen(text) + 4; // [ ] + space + text
    widget->height = 1;
    widget->text = kmalloc(strlen(text) + 1);
    if (widget->text) {
        strcpy(widget->text, text);
    }
    widget->active = checked;
    widget->callback = NULL;
    widget->next = window->widgets;
    
    window->widgets = widget;
    return widget;
}

// Draw terminal content in window
static void gui_draw_terminal_content(window_t* window) {
    if (!window || !window->is_terminal) return;
    
    int x = window->x + 1; // Start after border
    int y = window->y + GUI_TITLE_BAR_HEIGHT + 1; // Start after title bar and border
    int width = window->width - 2; // Account for borders
    int height = window->height - GUI_TITLE_BAR_HEIGHT - 2; // Account for title bar and borders
    
    vga_set_color(GUI_COLOR_WINDOW_FG, GUI_COLOR_WINDOW_BG);
    
    // Clear terminal area
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            vga_set_cursor(x + j, y + i);
            vga_putchar(' ');
        }
    }
    
    // Draw terminal content
    int line = 0;
    int col = 0;
    int pos = window->terminal_scroll;
    
    while (line < height && pos < GUI_TERMINAL_BUFFER_SIZE) {
        char c = window->terminal_buffer[pos];
        if (c == 0) break;
        
        if (c == '\n') {
            line++;
            col = 0;
        } else {
            if (col < width) {
                vga_set_cursor(x + col, y + line);
                vga_putchar(c);
                col++;
            }
        }
        pos++;
        
        // Wrap around buffer
        if (pos >= GUI_TERMINAL_BUFFER_SIZE) {
            pos = 0;
        }
    }
}

// Draw a window
void gui_draw_window(window_t* window) {
    if (!window || !window->visible) return;
    
    int x = window->x;
    int y = window->y;
    int width = window->width;
    int height = window->height;
    
    // Draw title bar
    vga_set_color(GUI_COLOR_TITLE_BAR_FG, GUI_COLOR_TITLE_BAR_BG);
    for (int i = 0; i < width; i++) {
        vga_set_cursor(x + i, y);
        vga_putchar(' ');
    }
    vga_set_cursor(x + 1, y);
    vga_write_string(window->title);
    
    // Draw window border
    vga_set_color(GUI_COLOR_BORDER_FG, GUI_COLOR_WINDOW_BG);
    for (int i = 0; i < width; i++) {
        vga_set_cursor(x + i, y + height - 1);
        vga_putchar('-');
    }
    for (int i = 1; i < height - 1; i++) {
        vga_set_cursor(x, y + i);
        vga_putchar('|');
        vga_set_cursor(x + width - 1, y + i);
        vga_putchar('|');
    }
    vga_set_cursor(x, y);
    vga_putchar('+');
    vga_set_cursor(x + width - 1, y);
    vga_putchar('+');
    vga_set_cursor(x, y + height - 1);
    vga_putchar('+');
    vga_set_cursor(x + width - 1, y + height - 1);
    vga_putchar('+');
    
    // Clear window interior or draw terminal content
    if (window->is_terminal) {
        // Draw terminal content
        gui_draw_terminal_content(window);
    } else {
        vga_set_color(GUI_COLOR_WINDOW_FG, GUI_COLOR_WINDOW_BG);
        for (int i = 1; i < height - 1; i++) {
            for (int j = 1; j < width - 1; j++) {
                vga_set_cursor(x + j, y + i);
                vga_putchar(' ');
            }
        }
    }
    
    // Draw widgets
    widget_t* widget = window->widgets;
    while (widget) {
        gui_draw_widget(widget, window);
        widget = widget->next;
    }
}
void gui_draw_widget(widget_t* widget, window_t* window) {
    if (!widget || !window) return;
    
    int wx = window->x + widget->x + 1; // +1 for border
    int wy = window->y + widget->y + GUI_TITLE_BAR_HEIGHT + 1; // +1 for title bar +1 for border
    
    switch (widget->type) {
        case WIDGET_BUTTON: {
            vga_color_t fg = widget->active ? GUI_COLOR_BUTTON_ACTIVE_FG : GUI_COLOR_BUTTON_FG;
            vga_color_t bg = widget->active ? GUI_COLOR_BUTTON_ACTIVE_BG : GUI_COLOR_BUTTON_BG;
            vga_set_color(fg, bg);
            
            // Draw button border
            for (int i = 0; i < widget->width; i++) {
                vga_set_cursor(wx + i, wy);
                vga_putchar(' ');
                vga_set_cursor(wx + i, wy + widget->height - 1);
                vga_putchar(' ');
            }
            for (int i = 0; i < widget->height; i++) {
                vga_set_cursor(wx, wy + i);
                vga_putchar(' ');
                vga_set_cursor(wx + widget->width - 1, wy + i);
                vga_putchar(' ');
            }
            
            // Draw button text
            int text_x = wx + (widget->width - strlen(widget->text)) / 2;
            int text_y = wy + widget->height / 2;
            vga_set_cursor(text_x, text_y);
            vga_write_string(widget->text);
            break;
        }
        
        case WIDGET_LABEL:
            vga_set_color(GUI_COLOR_WINDOW_FG, GUI_COLOR_WINDOW_BG);
            vga_set_cursor(wx, wy);
            vga_write_string(widget->text);
            break;
            
        case WIDGET_CHECKBOX:
            vga_set_color(GUI_COLOR_WINDOW_FG, GUI_COLOR_WINDOW_BG);
            vga_set_cursor(wx, wy);
            vga_putchar('[');
            vga_putchar(widget->active ? 'X' : ' ');
            vga_putchar(']');
            vga_putchar(' ');
            vga_write_string(widget->text);
            break;
            
        default:
            break;
    }
}

// Redraw all visible windows
void gui_redraw_all(void) {
    // Clear screen
    vga_clear();
    
    // Draw all visible windows (in order, focused window last)
    window_t* window = gui_state.windows;
    while (window) {
        if (window->visible) {
            gui_draw_window(window);
        }
        window = window->next;
    }
}

// Handle keyboard input for GUI
void gui_handle_input(char c) {
    // For now, just pass through to shell
    // Future: handle GUI-specific input
    (void)c;
}

// Placeholder for mouse input (future enhancement)
void gui_process_mouse(int x, int y, int button) {
    int prev_x = cursor_x;
    int prev_y = cursor_y;
    cursor_x = x;
    cursor_y = y;
    
    // Handle dragging
    if (drag_window && gui_state.focused_window) {
        // Move the window
        gui_state.focused_window->x = x - drag_offset_x;
        gui_state.focused_window->y = y - drag_offset_y;
        gui_redraw_all();
    }
    
    // Handle button presses
    if (button & 1) { // Left button down
        if (!drag_window) {
            // Check if click on window title bar
            window_t* window = gui_state.windows;
            while (window) {
                if (window->visible &&
                    x >= window->x && x < window->x + window->width &&
                    y >= window->y && y < window->y + GUI_TITLE_BAR_HEIGHT) {
                    // Start dragging
                    gui_focus_window(window);
                    drag_window = 1;
                    drag_offset_x = x - window->x;
                    drag_offset_y = y - window->y;
                    break;
                }
                window = window->next;
            }
        }
    } else { // Left button up
        drag_window = 0;
    }
    
    // Draw cursor
    gui_draw_cursor();
}

// Draw mouse cursor
void gui_draw_cursor(void) {
    // In text mode, print cursor char at position
    int tx = cursor_x / 8;
    int ty = cursor_y / 16;
    if (tx >= 0 && tx < VGA_WIDTH && ty >= 0 && ty < VGA_HEIGHT) {
        vga_set_cursor(tx, ty);
        vga_putchar('*');
    }
}

// Terminal functions
void gui_terminal_write(window_t* window, const char* str) {
    if (!window || !window->is_terminal) return;
    
    while (*str) {
        gui_terminal_putchar(window, *str);
        str++;
    }
}

void gui_terminal_putchar(window_t* window, char c) {
    if (!window || !window->is_terminal) return;
    
    if (c == '\n') {
        // Handle newline
        window->terminal_buffer[window->terminal_pos++] = '\n';
        if (window->terminal_pos >= GUI_TERMINAL_BUFFER_SIZE) {
            window->terminal_pos = 0; // Wrap around
        }
    } else if (c == '\b') {
        // Handle backspace
        if (window->terminal_pos > 0) {
            window->terminal_pos--;
        }
    } else if (c >= 32 && c <= 126) {
        // Printable character
        window->terminal_buffer[window->terminal_pos++] = c;
        if (window->terminal_pos >= GUI_TERMINAL_BUFFER_SIZE) {
            window->terminal_pos = 0; // Wrap around
        }
    }
    
    // Redraw terminal content
    gui_draw_window(window);
}

void gui_terminal_clear(window_t* window) {
    if (!window || !window->is_terminal) return;
    
    memset(window->terminal_buffer, 0, GUI_TERMINAL_BUFFER_SIZE);
    window->terminal_pos = 0;
    window->terminal_scroll = 0;
    gui_draw_window(window);
}

// Search terminal buffer for query
void gui_terminal_search(window_t* window, const char* query) {
    if (!window || !window->is_terminal || !query) return;
    
    // Simple search implementation - highlight matches
    // For now, just scroll to show search results
    // Future: implement proper highlighting
    
    int query_len = strlen(query);
    if (query_len == 0) return;
    
    // Search through buffer
    for (int i = 0; i < window->terminal_pos - query_len; i++) {
        if (memcmp(&window->terminal_buffer[i], query, query_len) == 0) {
            // Found match, scroll to show it
            window->terminal_scroll = i / GUI_TERMINAL_WIDTH;
            gui_draw_window(window);
            break;
        }
    }
}