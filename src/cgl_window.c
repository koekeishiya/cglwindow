#include "cgl_window.h"

enum CGSWindowBackingType
{
	kCGSBackingNonRetained = 0,
	kCGSBackingRetained = 1,
	kCGSBackingBuffered = 2
};

enum CGSWindowOrderingMode
{
   kCGSOrderAbove = 1,
   kCGSOrderBelow = -1,
   kCGSOrderOut = 0
};

#ifdef __cplusplus
extern "C" {
#endif
CGSConnectionID CGSMainConnectionID(void);
CGError CGSNewWindow(CGSConnectionID cid, int, float, float, const CGSRegionRef, CGSWindowID *);
CGError CGSReleaseWindow(CGSConnectionID cid, CGWindowID wid);
CGError CGSNewRegionWithRect(const CGRect * rect, CGSRegionRef *newRegion);
OSStatus CGSOrderWindow(CGSConnectionID cid, CGSWindowID wid, enum CGSWindowOrderingMode place, CGSWindowID relativeToWindow /* nullable */);
CGError CGSMoveWindow(CGSConnectionID cid, CGSWindowID wid, CGPoint *window_pos);
CGError CGSSetWindowOpacity(CGSConnectionID cid, CGSWindowID wid, bool isOpaque);
CGError CGSSetWindowLevel(CGSConnectionID cid, CGSWindowID wid, CGWindowLevel level);
CGError CGSAddSurface(CGSConnectionID cid, CGSWindowID wid, CGSSurfaceID *sid);
CGError CGSSetSurfaceBounds(CGSConnectionID cid, CGSWindowID wid, CGSSurfaceID sid, CGRect rect);
CGError CGSOrderSurface(CGSConnectionID cid, CGSWindowID wid, CGSSurfaceID sid, int a, int b);
CGLError CGLSetSurface(CGLContextObj gl, CGSConnectionID cid, CGSWindowID wid, CGSSurfaceID sid);
CGContextRef CGWindowContextCreate(CGSConnectionID cid, CGSWindowID wid, CFDictionaryRef options);
#ifdef __cplusplus
}
#endif

static int
cgl_window_context_init(struct cgl_window *window, int use_legacy_gl)
{
    CGLPixelFormatObj pixel_format;
    GLint surface_opacity;
    GLint v_sync_enabled;
    CGLError cgl_error;
    CGError cg_error;
    GLint drawable;
    GLint num;

    CGLPixelFormatAttribute gl_profile_version = use_legacy_gl
                                               ? (CGLPixelFormatAttribute) kCGLOGLPVersion_Legacy
                                               : (CGLPixelFormatAttribute) kCGLOGLPVersion_3_2_Core;
    CGLPixelFormatAttribute attributes[] = {
        kCGLPFADoubleBuffer,
        kCGLPFAAccelerated,
        kCGLPFAOpenGLProfile,
        gl_profile_version,
        0
    };

    CGLChoosePixelFormat(attributes, &pixel_format, &num);
    if(!pixel_format) {
        goto err;
    }

    CGLCreateContext(pixel_format, NULL, &window->context);
    if(!window->context) {
        goto err_pix_fmt;
    }

    v_sync_enabled = 1;
    CGLSetParameter(window->context, kCGLCPSwapInterval, &v_sync_enabled);

    surface_opacity = 0;
    CGLSetParameter(window->context, kCGLCPSurfaceOpacity, &surface_opacity);

    cg_error = CGSAddSurface(window->connection, window->id, &window->surface);
    if(cg_error != kCGErrorSuccess) {
        goto err_context;
    }

    cg_error = CGSSetSurfaceBounds(window->connection, window->id, window->surface, CGRectMake(0, 0, window->width, window->height));
    if(cg_error != kCGErrorSuccess) {
        goto err_context;
    }

    cg_error = CGSOrderSurface(window->connection, window->id, window->surface, 1, 0);
    if(cg_error != kCGErrorSuccess) {
        goto err_context;
    }

    cgl_error = CGLSetSurface(window->context, window->connection, window->id, window->surface);
    if(cgl_error != kCGLNoError) {
        goto err_context;
    }

    CGLGetParameter(window->context, kCGLCPHasDrawable, &drawable);
    if(!drawable) {
        goto err_context;
    }

    CGLDestroyPixelFormat(pixel_format);
    return 1;

err_context:
    CGLDestroyContext(window->context);

err_pix_fmt:
    CGLDestroyPixelFormat(pixel_format);

err:
    return 0;
}

int cgl_window_init(struct cgl_window *window, CGFloat x, CGFloat y, CGFloat width, CGFloat height, int level, int use_legacy_gl)
{
    CGContextRef context;
    CGSRegionRef region;
    CGRect rect;
    int result = 0;

    window->connection = CGSMainConnectionID();
    if(!window->connection) {
        goto err;
    }

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->level = level;

    rect = CGRectMake(0, 0, window->width, window->height);
    CGSNewRegionWithRect(&rect, &region);
    if(!region) {
        goto err;
    }

    CGSNewWindow(window->connection, kCGSBackingBuffered, window->x, window->y, region, &window->id);
    if(!window->id) {
        goto err_region;
    }

    CGSSetWindowOpacity(window->connection, window->id, 0);
    CGSSetWindowLevel(window->connection, window->id, CGWindowLevelForKey((CGWindowLevelKey)window->level));
    CGSOrderWindow(window->connection, window->id, kCGSOrderAbove, 0);

    context = CGWindowContextCreate(window->connection, window->id, 0);
    CGContextClearRect(context, rect);
    CGContextRelease(context);

    result = cgl_window_context_init(window, use_legacy_gl);

err_region:
    CFRelease(region);

err:
    return result;
}

void cgl_window_destroy(struct cgl_window *window)
{
    CGLDestroyContext(window->context);
    CGSReleaseWindow(window->connection, window->id);
}

void cgl_window_make_current(struct cgl_window *window)
{
    CGLSetCurrentContext(window->context);
}

CGLError cgl_window_flush(struct cgl_window *window)
{
    return CGLFlushDrawable(window->context);
}

void cgl_window_set_input_callback(struct cgl_window *window, cgl_window_input_callback *callback)
{
    window->input_callback = callback;
}

void cgl_window_process_input_events(struct cgl_window *window)
{
    EventTargetRef event_target = GetEventDispatcherTarget();
    EventRef event_ref;
    CGEventRef event;

    while (ReceiveNextEvent(0, NULL, kEventDurationNoWait, true, &event_ref) == noErr) {
        if ((event = CopyEventCGEvent(event_ref))) {
            if (window->input_callback) {
                window->input_callback(window, event);
            }
            CFRelease(event);
        }

        SendEventToEventTarget(event_ref, event_target);
        ReleaseEvent(event_ref);
    }
}
