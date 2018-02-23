#ifndef CGL_WINDOW_H
#define CGL_WINDOW_H

#include <Carbon/Carbon.h>
#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#include <stdint.h>

typedef int CGSConnectionID;
typedef uint32_t CGSWindowID;
typedef int CGWindowLevel;
typedef int CGSSurfaceID;
typedef CFTypeRef CGSRegionRef;

struct cgl_window;
#define CGL_WINDOW_INPUT_CALLBACK(name) void name(struct cgl_window *window, CGEventRef event)
typedef CGL_WINDOW_INPUT_CALLBACK(cgl_window_input_callback);

enum cgl_window_gl_profile
{
    CGL_WINDOW_GL_LEGACY = 0,
    CGL_WINDOW_GL_CORE   = 1
};

struct cgl_window
{
    CGSConnectionID connection;
    CGSWindowID id;
    ProcessSerialNumber psn;
    CGLContextObj context;
    CGSSurfaceID surface;
    CGWindowLevel level;
    CGFloat x, y, width, height;
    cgl_window_input_callback *input_callback;
};

void cgl_window_process_input_events(struct cgl_window *window);
void cgl_window_bring_to_front(struct cgl_window *window);

int cgl_window_init(struct cgl_window *window, enum cgl_window_gl_profile gl_profile, cgl_window_input_callback *callback, CGFloat x, CGFloat y, CGFloat width, CGFloat height, int level);
void cgl_window_destroy(struct cgl_window *window);

void cgl_window_make_current(struct cgl_window *window);
CGLError cgl_window_flush(struct cgl_window *window);

#endif
