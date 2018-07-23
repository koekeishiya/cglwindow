#ifndef CGL_WINDOW_H
#define CGL_WINDOW_H

#define GL_SILENCE_DEPRECATION

#include <Carbon/Carbon.h>
#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>
#include <OpenGL/gl.h>
#include <stdint.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef int CGSConnectionID;
typedef uint32_t CGSWindowID;
typedef int CGWindowLevel;
typedef int CGSSurfaceID;
typedef CFTypeRef CGSRegionRef;

struct cgl_window;
#define CGL_WINDOW_EVENT_CALLBACK(name) void name(struct cgl_window *window, EventRef event, void *user_data)
typedef CGL_WINDOW_EVENT_CALLBACK(cgl_window_event_callback);

enum cgl_window_event_modifier
{
    CGL_EVENT_MOD_CMD   = cmdKey,
    CGL_EVENT_MOD_SHIFT = shiftKey,
    CGL_EVENT_MOD_CAPS  = alphaLock,
    CGL_EVENT_MOD_ALT   = optionKey,
    CGL_EVENT_MOD_CTRL  = controlKey,
    CGL_EVENT_MOD_NUM   = kEventKeyModifierNumLockMask,
    CGL_EVENT_MOD_FN    = kEventKeyModifierFnMask
};

enum cgl_window_gl_profile
{
    CGL_WINDOW_GL_LEGACY = 0,
    CGL_WINDOW_GL_CORE   = 1
};

enum cgl_window_level
{
    CGL_WINDOW_BASE_LEVEL                =  0,
    CGL_WINDOW_MINIMUM_LEVEL             =  1,
    CGL_WINDOW_DESKTOP_LEVEL             =  2,
    CGL_WINDOW_BACKSTOP_MENU_LEVEL       =  3,
    CGL_WINDOW_NORMAL_LEVEL              =  4,
    CGL_WINDOW_FLOATING_LEVEL            =  5,
    CGL_WINDOW_TORN_OFF_MENU_LEVEL       =  6,
    CGL_WINDOW_DOCK_LEVEL                =  7,
    CGL_WINDOW_MAIN_MENU_LEVEL           =  8,
    CGL_WINDOW_STATUS_LEVEL              =  9,
    CGL_WINDOW_MODAL_PANEL_LEVEL         = 10,
    CGL_WINDOW_POPUP_MENU_LEVEL          = 11,
    CGL_WINDOW_DRAGGING_LEVEL            = 12,
    CGL_WINDOW_SCREENSAVER_LEVEL         = 13,
    CGL_WINDOW_MAXIMUM_LEVEL             = 14,
    CGL_WINDOW_OVERLAY_LEVEL             = 15,
    CGL_WINDOW_HELP_LEVEL                = 16,
    CGL_WINDOW_UTILITY_LEVEL             = 17,
    CGL_WINDOW_DESKTOP_ICON_LEVEL        = 18,
    CGL_WINDOW_CURSOR_LEVEL              = 19,
    CGL_WINDOW_ASSISTIVE_TECH_HIGH_LEVEL = 20
};

struct cgl_window
{
    CGSConnectionID connection;
    CGSWindowID id;
    ProcessSerialNumber psn;
    CGLContextObj context;
    CGSSurfaceID surface;
    enum cgl_window_level level;
    CGFloat x, y, width, height;
    GLint v_sync;
    enum cgl_window_gl_profile gl_profile;
    cgl_window_event_callback *mouse_callback;
    cgl_window_event_callback *key_callback;
    cgl_window_event_callback *application_callback;
};

void cgl_window_set_application_callback(struct cgl_window *window, cgl_window_event_callback *mouse_callback);
void cgl_window_set_mouse_callback(struct cgl_window *window, cgl_window_event_callback *mouse_callback);
void cgl_window_set_key_callback(struct cgl_window *window, cgl_window_event_callback *key_callback);

void cgl_window_poll_events(struct cgl_window *window, void *user_data);
void cgl_window_bring_to_front(struct cgl_window *window);
void cgl_window_show_cursor(struct cgl_window *window, bool visible);
bool cgl_window_toggle_fullscreen(struct cgl_window *window);

bool cgl_window_init(struct cgl_window *window, CGFloat x, CGFloat y, CGFloat width, CGFloat height, enum cgl_window_level level, enum cgl_window_gl_profile gl_profile, bool v_sync);
void cgl_window_destroy(struct cgl_window *window);

bool cgl_window_move(struct cgl_window *window, float x, float y);
bool cgl_window_resize(struct cgl_window *window, float width, float height);
void cgl_window_set_alpha(struct cgl_window *window, float alpha);
void cgl_window_set_level(struct cgl_window *window, enum cgl_window_level level);
void cgl_window_set_sticky(struct cgl_window *window, bool sticky);
void cgl_window_add_drag_region(struct cgl_window *window, float x, float y, float width, float height);
void cgl_window_clear_drag_region(struct cgl_window *window);

void cgl_window_make_current(struct cgl_window *window);
CGLError cgl_window_flush(struct cgl_window *window);

#endif
