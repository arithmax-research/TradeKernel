#include "gui.h"
#include "mm/memory.h"
#include "shell.h"

// Global GUI state
static gui_state_t gui_state;
static int desktop_active = 0;
static int launcher_open = 0;
static int launcher_selection = 0;
static window_t* app_windows[4] = {NULL, NULL, NULL, NULL};

static const char* launcher_apps[4] = {
    "Terminal",
    "System Monitor",
    "Network",
    "About TradeKernel"
};

static void gui_draw_desktop(void) {
    vga_set_color(GUI_COLOR_DESKTOP_FG, GUI_COLOR_DESKTOP_BG);
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_set_cursor(x, y);
            vga_putchar(' ');
        }
    }

    // Subtle text-mode texture every 4 columns to avoid a flat background.
    vga_set_color(VGA_COLOR_LIGHT_BLUE, GUI_COLOR_DESKTOP_BG);
    for (int y = 2; y < VGA_HEIGHT; y += 2) {
        for (int x = 0; x < VGA_WIDTH; x += 4) {
            vga_set_cursor(x, y);
            vga_putchar('.');
        }
    }
}

static void gui_draw_top_panel(void) {
    vga_set_color(GUI_COLOR_PANEL_FG, GUI_COLOR_PANEL_BG);
    vga_set_cursor(0, 0);
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_putchar(' ');
    }

    vga_set_cursor(1, 0);
    vga_write_string("TradeKernel  [F1] Launcher  [F2] Shell");

    vga_set_cursor(VGA_WIDTH - 18, 0);
    if (launcher_open) {
        vga_write_string("Launcher: Open");
    } else {
        vga_write_string("Launcher: Closed");
    }
}

static void gui_draw_launcher_menu(void) {
    if (!launcher_open) {
        return;
    }

    const int menu_x = 2;
    const int menu_y = 2;
    const int menu_w = 34;
    const int menu_h = 11;

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    for (int y = 0; y < menu_h; y++) {
        for (int x = 0; x < menu_w; x++) {
            vga_set_cursor(menu_x + x, menu_y + y);
            vga_putchar(' ');
        }
    }

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY);
    for (int x = 0; x < menu_w; x++) {
        vga_set_cursor(menu_x + x, menu_y);
        vga_putchar(' ');
    }
    vga_set_cursor(menu_x + 1, menu_y);
    vga_write_string("Applications");

    for (int i = 0; i < 4; i++) {
        vga_set_cursor(menu_x + 2, menu_y + 2 + i * 2);
        if (launcher_selection == i) {
            vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN);
        } else {
            vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }
        vga_write_string("                                ");
        vga_set_cursor(menu_x + 3, menu_y + 2 + i * 2);
        vga_putchar('1' + i);
        vga_write_string(". ");
        vga_write_string(launcher_apps[i]);
    }

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_set_cursor(menu_x + 2, menu_y + menu_h - 1);
    vga_write_string("Use arrows + Enter, Esc to close");
}

static void gui_open_app(int app_index) {
    window_t* app = app_windows[app_index];

    if (!app) {
        switch (app_index) {
            case 0:
                app = gui_create_window(16, 5, 48, 12, "Terminal");
                if (app) {
                    gui_create_label(app, 2, 1, "Shell is still active in kernel mode.");
                    gui_create_label(app, 2, 3, "Use F2 to return to shell prompt.");
                    gui_create_label(app, 2, 5, "This is a desktop launcher prototype.");
                }
                break;
            case 1:
                app = gui_create_window(14, 4, 52, 14, "System Monitor");
                if (app) {
                    gui_create_label(app, 2, 1, "CPU: scheduler tick active");
                    gui_create_label(app, 2, 3, "Memory: use mem/memstats in shell");
                    gui_create_label(app, 2, 5, "Processes: use ps/procinfo in shell");
                }
                break;
            case 2:
                app = gui_create_window(18, 6, 44, 11, "Network");
                if (app) {
                    gui_create_label(app, 2, 1, "Network stack initialized");
                    gui_create_label(app, 2, 3, "Use wstest for live WebSocket test");
                    gui_create_label(app, 2, 5, "Use future netstat command for stats");
                }
                break;
            case 3:
                app = gui_create_window(12, 5, 56, 13, "About TradeKernel");
                if (app) {
                    gui_create_label(app, 2, 1, "TradeKernel OS - text desktop preview");
                    gui_create_label(app, 2, 3, "Goal: evolve into framebuffer compositor");
                    gui_create_label(app, 2, 5, "Now includes a panel and app launcher");
                }
                break;
            default:
                break;
        }

        if (app) {
            app_windows[app_index] = app;
        }
    }

    if (app) {
        gui_show_window(app);
    }
}

// Initialize GUI system
void gui_init(void) {
    memset(&gui_state, 0, sizeof(gui_state_t));
    gui_state.next_window_id = 1;
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
    window->widgets = NULL;
    window->next = gui_state.windows;
    
    gui_state.windows = window;
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

// Draw a window
void gui_draw_window(window_t* window) {
    if (!window || !window->visible) return;
    
    int x = window->x;
    int y = window->y;
    int width = window->width;
    int height = window->height;
    
    // Draw title bar with focused/unfocused visual state.
    vga_color_t title_fg = window->focused ? GUI_COLOR_TITLE_BAR_FG : GUI_COLOR_TITLE_BAR_UNFOCUSED_FG;
    vga_color_t title_bg = window->focused ? GUI_COLOR_TITLE_BAR_BG : GUI_COLOR_TITLE_BAR_UNFOCUSED_BG;

    vga_set_color(title_fg, title_bg);
    for (int i = 0; i < width; i++) {
        vga_set_cursor(x + i, y);
        vga_putchar(' ');
    }
    vga_set_cursor(x + 1, y);
    vga_write_string(window->title);

    if (width > 14) {
        vga_set_cursor(x + width - 12, y);
        vga_write_string("[_][#][X]");
    }
    
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

    // Draw a simple shadow to improve depth perception.
    vga_set_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    if (x + width < VGA_WIDTH) {
        for (int i = 1; i < height; i++) {
            vga_set_cursor(x + width, y + i);
            vga_putchar(' ');
        }
    }
    if (y + height < VGA_HEIGHT) {
        for (int i = 1; i <= width; i++) {
            if (x + i < VGA_WIDTH) {
                vga_set_cursor(x + i, y + height);
                vga_putchar(' ');
            }
        }
    }
    
    // Clear window interior
    vga_set_color(GUI_COLOR_WINDOW_FG, GUI_COLOR_WINDOW_BG);
    for (int i = 1; i < height - 1; i++) {
        for (int j = 1; j < width - 1; j++) {
            vga_set_cursor(x + j, y + i);
            vga_putchar(' ');
        }
    }
    
    // Draw widgets
    widget_t* widget = window->widgets;
    while (widget) {
        gui_draw_widget(widget, window);
        widget = widget->next;
    }
}

// Draw a widget
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
    gui_draw_desktop();
    gui_draw_top_panel();
    
    // Draw all visible windows (in order, focused window last).
    window_t* window = gui_state.windows;
    while (window) {
        if (window->visible) {
            gui_draw_window(window);
        }
        window = window->next;
    }

    gui_draw_launcher_menu();
}

// Handle keyboard input for GUI
void gui_handle_input(char c) {
    // For now, just pass through to shell
    // Future: handle GUI-specific input
    (void)c;
}

void gui_enter_desktop(void) {
    desktop_active = 1;
    launcher_open = 1;
    launcher_selection = 0;
    gui_redraw_all();
}

void gui_exit_desktop(void) {
    desktop_active = 0;
    launcher_open = 0;
    vga_clear();
    shell_init();
}

int gui_is_desktop_active(void) {
    return desktop_active;
}

int gui_handle_scancode(uint8_t scancode) {
    // F1 opens desktop launcher from shell mode, or toggles launcher in desktop mode.
    if (scancode == 0x3B) {
        if (!desktop_active) {
            gui_enter_desktop();
        } else {
            launcher_open = !launcher_open;
            gui_redraw_all();
        }
        return 1;
    }

    if (!desktop_active) {
        return 0;
    }

    // While desktop is active, GUI owns all keyboard input.
    if (scancode == 0x3C) {
        gui_exit_desktop();
        return 1;
    }

    if (scancode == 0x01) {
        if (launcher_open) {
            launcher_open = 0;
            gui_redraw_all();
        } else {
            gui_exit_desktop();
        }
        return 1;
    }

    if (scancode == 0x48 && launcher_open) {
        if (launcher_selection > 0) launcher_selection--;
        gui_redraw_all();
        return 1;
    }

    if (scancode == 0x50 && launcher_open) {
        if (launcher_selection < 3) launcher_selection++;
        gui_redraw_all();
        return 1;
    }

    if ((scancode >= 0x02 && scancode <= 0x05) && launcher_open) {
        launcher_selection = scancode - 0x02;
        gui_open_app(launcher_selection);
        launcher_open = 0;
        gui_redraw_all();
        return 1;
    }

    if (scancode == 0x1C && launcher_open) {
        gui_open_app(launcher_selection);
        launcher_open = 0;
        gui_redraw_all();
        return 1;
    }

    if (scancode == 0x39) {
        launcher_open = !launcher_open;
        gui_redraw_all();
        return 1;
    }

    return 1;
}

// Placeholder for mouse input (future enhancement)
void gui_process_mouse(int x, int y, int button) {
    (void)x; (void)y; (void)button;
    // Future: handle mouse clicks on widgets
}