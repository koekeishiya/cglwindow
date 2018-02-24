#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cgl_window.h"
#include "cgl_window.c"

#define global_variable static
#define local_persist static
#define internal static

global_variable bool should_quit;
global_variable bool left_mouse_down;

void key_callback(struct cgl_window *window, EventRef event)
{
    uint32_t event_kind = GetEventKind(event);
    if ((event_kind == kEventRawKeyDown) ||
        (event_kind == kEventRawKeyRepeat) ||
        (event_kind == kEventRawKeyUp)) {
        uint32_t keycode;
        char charcode;
        uint32_t modifiers;

        GetEventParameter(event, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &keycode);
        GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &charcode);
        GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, 0, sizeof(UInt32), 0, &modifiers);

        printf("modifiers: %08x, keycode: %d, charcode: %c\n", modifiers, keycode, charcode);

        if (event_kind == kEventRawKeyDown && (keycode == kVK_ANSI_Q || keycode == kVK_Escape)) {
            should_quit = true;
        }

        if (event_kind == kEventRawKeyDown && keycode == kVK_ANSI_R && (modifiers & CGL_EVENT_MOD_SHIFT)) {
            cgl_window_resize(window, window->x, window->y, window->width - 5, window->height - 5);
        } else if (event_kind == kEventRawKeyDown && keycode == kVK_ANSI_R) {
            cgl_window_resize(window, window->x, window->y, window->width + 5, window->height + 5);
        }
    }
}

void mouse_callback(struct cgl_window *window, EventRef event)
{
    uint32_t event_kind = GetEventKind(event);
    if ((event_kind == kEventMouseDown) || (event_kind == kEventMouseUp) ||
        (event_kind == kEventMouseDragged) || (event_kind == kEventMouseMoved)) {
        EventMouseButton button;
        uint32_t modifiers;
        HIPoint location;
        HIPoint delta;

        GetEventParameter(event, kEventParamMouseButton, typeMouseButton, 0, sizeof(EventMouseButton), 0, &button);
        GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, 0, sizeof(UInt32), 0, &modifiers);
        GetEventParameter(event, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(HIPoint), NULL, &location);
        GetEventParameter(event, kEventParamMouseDelta, typeHIPoint, NULL, sizeof(HIPoint), NULL, &delta);

        printf("modifiers: %08x, button: %d, location.x: %.2f, location.y: %.2f, delta.x: %.2f, delta.y: %.2f\n", modifiers, button, location.x, location.y, delta.x, delta.y);

        if (event_kind == kEventMouseDown && button == 1) {
            left_mouse_down = true;
            cgl_window_bring_to_front(window);
        } else if (event_kind == kEventMouseUp && button == 1) {
            left_mouse_down = false;
        } else if (event_kind == kEventMouseDragged && button == 1) {
           cgl_window_move(window, window->x + delta.x, window->y + delta.y);
        } else if (event_kind == kEventMouseDown && button == 2) {
            should_quit = true;
        }
    }
}

void render_triangle(struct cgl_window *window)
{
    glClearColor((int)left_mouse_down, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    static float a = 0;
    glRotatef(a * 1000, 0, 0, 1);
    a = a + 0.001;
    glBegin(GL_QUADS);
    if (a > 1.5) {
        a = 0;
    }
    glColor4f(a, 1, 0, 1);
    glVertex2f(0.25, 0.25);
    glVertex2f(0.75, 0.25);
    glVertex2f(0.75, 0.75);
    glVertex2f(0.25, 0.75);
    glEnd();
}

int main(int argc, char **argv)
{
    struct cgl_window window = {};
    if (cgl_window_init(&window, 200, 200, 500, 500, kCGNormalWindowLevelKey, CGL_WINDOW_GL_LEGACY)) {
        cgl_window_set_mouse_callback(&window, &mouse_callback);
        cgl_window_set_key_callback(&window, &key_callback);
        cgl_window_make_current(&window);

        while (!should_quit) {
            cgl_window_process_events(&window);
            render_triangle(&window);
            cgl_window_flush(&window);
        }

        cgl_window_destroy(&window);
    }

    return 0;
}
