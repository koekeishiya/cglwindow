#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cgl_window.h"
#include "cgl_window.c"

#define global_variable static
#define local_persist static
#define internal static

global_variable bool should_quit;
global_variable bool mouse_is_down;
global_variable bool mouse_drag_started;
global_variable CGPoint mouse_drag_start_pos;

CGL_WINDOW_INPUT_CALLBACK(input_callback)
{
    char *event_type_str = NULL;
    CGEventType event_type = CGEventGetType(event);
    switch (event_type) {
    case kCGEventLeftMouseDown: {
        event_type_str = "kCGEventLeftMouseDown";
        mouse_is_down = true;
        cgl_window_bring_to_front(window);
    } break;
    case kCGEventLeftMouseUp: {
        event_type_str = "kCGEventLeftMouseUp";
        mouse_is_down = false;
        mouse_drag_started = false;
    } break;
    case kCGEventRightMouseDown: {
        event_type_str = "kCGEventRightMouseDown";
        should_quit = true;
    } break;
    case kCGEventRightMouseUp: {
        event_type_str = "kCGEventRightMouseUp";
    } break;
    case kCGEventMouseMoved: {
        event_type_str = "kCGEventMouseMoved";
    } break;
    case kCGEventLeftMouseDragged: {
       event_type_str = "kCGEventLeftMouseDragged";
       CGPoint event_pos = CGEventGetLocation(event);

       if (!mouse_drag_started) {
           mouse_drag_started = true;
           mouse_drag_start_pos = event_pos;
       }

       cgl_window_move(window, window->x + event_pos.x - mouse_drag_start_pos.x, window->y + event_pos.y - mouse_drag_start_pos.y);
       mouse_drag_start_pos = event_pos;
    } break;
    case kCGEventRightMouseDragged: {
        event_type_str = "kCGEventRightMouseDragged";
    } break;
    case kCGEventKeyDown: {
        event_type_str = "kCGEventKeyDown";
        uint32_t flags = CGEventGetFlags(event);
        uint32_t key = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

        if (key == kVK_ANSI_Q || key == kVK_Escape) {
            should_quit = true;
        }

        if (key == kVK_ANSI_R) {
            int r = cgl_window_resize(window, window->x, window->y, window->width + 5, window->height + 5);
            printf("resize result: %d\n", r);
        }

        printf("keycode: 0x%02x, flags: %08x\t", key, flags);
    } break;
    case kCGEventKeyUp: {
        event_type_str = "kCGEventKeyUp";
    } break;
    case kCGEventFlagsChanged: {
        event_type_str = "kCGEventFlagsChanged";
    } break;
    case kCGEventScrollWheel: {
        event_type_str = "kCGEventScrollWheel";
    } break;
    case kCGEventTabletPointer: {
        event_type_str = "kCGEventTabletPointer";
    } break;
    case kCGEventTabletProximity: {
        event_type_str = "kCGEventTabletProximity";
    } break;
    case kCGEventOtherMouseDown: {
        event_type_str = "kCGEventOtherMouseDown";
    } break;
    case kCGEventOtherMouseUp: {
        event_type_str = "kCGEventOtherMouseUp";
    } break;
    case kCGEventOtherMouseDragged: {
        event_type_str = "kCGEventOtherMouseDragged";
    } break;
    case kCGEventTapDisabledByTimeout: {
        event_type_str = "kCGEventTapDisabledByTimeout";
    } break;
    case kCGEventTapDisabledByUserInput: {
        event_type_str = "kCGEventTapDisabledByUserInput";
    } break;
    default: {
        event_type_str = "unknown event";
    } break;
    }

    printf("%s:%d\n", event_type_str, event_type);
}

void render_triangle(struct cgl_window *window)
{
    glClearColor((int)mouse_is_down, 0, 0, 1);
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
    if (cgl_window_init(&window, 200, 200, 500, 500, kCGNormalWindowLevelKey, CGL_WINDOW_GL_LEGACY, &input_callback)) {
        cgl_window_make_current(&window);

        while (!should_quit) {
            cgl_window_process_input_events(&window);
            render_triangle(&window);
            cgl_window_flush(&window);
        }

        cgl_window_destroy(&window);
    }

    return 0;
}
