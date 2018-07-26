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
CGError CGSSetWindowShape(CGSConnectionID cid, CGWindowID wid, float x_offset, float y_offset, const CGSRegionRef shape);
OSStatus CGSOrderWindow(CGSConnectionID cid, CGSWindowID wid, enum CGSWindowOrderingMode place, CGSWindowID relativeToWindow /* nullable */);
CGError CGSMoveWindow(CGSConnectionID cid, CGSWindowID wid, CGPoint *window_pos);
CGError CGSSetWindowOpacity(CGSConnectionID cid, CGSWindowID wid, bool isOpaque);
CGError CGSSetWindowAlpha(CGSConnectionID cid, CGSWindowID wid, float alpha);
CGError CGSSetWindowLevel(CGSConnectionID cid, CGSWindowID wid, CGWindowLevel level);
CGError CGSAddSurface(CGSConnectionID cid, CGSWindowID wid, CGSSurfaceID *sid);
CGError CGSRemoveSurface(CGSConnectionID cid, CGSWindowID wid, CGSSurfaceID sid);
CGError CGSSetSurfaceBounds(CGSConnectionID cid, CGSWindowID wid, CGSSurfaceID sid, CGRect rect);
CGError CGSOrderSurface(CGSConnectionID cid, CGSWindowID wid, CGSSurfaceID sid, int a, int b);
CGLError CGLSetSurface(CGLContextObj gl, CGSConnectionID cid, CGSWindowID wid, CGSSurfaceID sid);
CGContextRef CGWindowContextCreate(CGSConnectionID cid, CGSWindowID wid, CFDictionaryRef options);
CGError CGSSetWindowTags(CGSConnectionID cid, CGSWindowID wid, const int tags[2], size_t tag_size);
CGError CGSClearWindowTags(CGSConnectionID cid, CGSWindowID wid, const int tags[2], size_t tag_size);
CGError CGSAddActivationRegion(CGSConnectionID cid, CGSWindowID wid, CGSRegionRef region);
CGError CGSClearActivationRegion(CGSConnectionID cid, CGSWindowID wid);
CGError CGSAddDragRegion(CGSConnectionID cid, CGSWindowID wid, CGSRegionRef region);
CGError CGSClearDragRegion(CGSConnectionID cid, CGSWindowID wid);
CGError CGSAddTrackingRect(CGSConnectionID cid, CGSWindowID wid, CGRect rect);
CGError CGSRemoveAllTrackingAreas(CGSConnectionID cid, CGSWindowID wid);
CFStringRef CGSCopyManagedDisplayForWindow(const CGSConnectionID cid, CGSWindowID wid);
CGError CGSGetScreenRectForWindow(CGSConnectionID cid, CGSWindowID wid, CGRect *outRect);
#ifdef __cplusplus
}
#endif

static int cgl_gl_profiles[2] = { kCGLOGLPVersion_Legacy, kCGLOGLPVersion_3_2_Core };

static bool
cgl_window_surface_init(struct cgl_window *window)
{
    GLint surface_opacity = 0;
    CGLSetParameter(window->context, kCGLCPSurfaceOpacity, &surface_opacity);

    if (CGSAddSurface(window->connection, window->id, &window->surface) != kCGErrorSuccess) {
        goto err;
    }

    if (CGSSetSurfaceBounds(window->connection, window->id, window->surface, CGRectMake(0, 0, window->width, window->height)) != kCGErrorSuccess) {
        goto err_surface;
    }

    if (CGSOrderSurface(window->connection, window->id, window->surface, 1, 0) != kCGErrorSuccess) {
        goto err_surface;
    }

    if (CGLSetSurface(window->context, window->connection, window->id, window->surface) != kCGLNoError) {
        goto err_surface;
    }

    return true;

err_surface:
    CGSRemoveSurface(window->connection, window->id, window->surface);

err:
    return false;
}

static bool
cgl_window_context_init(struct cgl_window *window)
{
    CGLPixelFormatObj pixel_format;
    GLint drawable;
    GLint num;

    CGLPixelFormatAttribute attributes[] = {
        kCGLPFADoubleBuffer,
        kCGLPFAAccelerated,
        kCGLPFAOpenGLProfile,
        (CGLPixelFormatAttribute) cgl_gl_profiles[window->gl_profile],
        (CGLPixelFormatAttribute) 0
    };

    CGLChoosePixelFormat(attributes, &pixel_format, &num);
    if(!pixel_format) {
        goto err;
    }

    CGLCreateContext(pixel_format, 0, &window->context);
    if(!window->context) {
        goto err_pix_fmt;
    }

    CGLSetParameter(window->context, kCGLCPSwapInterval, &window->v_sync);

    if (!cgl_window_surface_init(window)) {
        goto err_context;
    }

    CGLGetParameter(window->context, kCGLCPHasDrawable, &drawable);
    if(!drawable) {
        goto err_context;
    }

    CGLDestroyPixelFormat(pixel_format);
    return true;

err_context:
    CGLDestroyContext(window->context);

err_pix_fmt:
    CGLDestroyPixelFormat(pixel_format);

err:
    return false;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
bool cgl_window_init(struct cgl_window *window, CGFloat x, CGFloat y, CGFloat width, CGFloat height, enum cgl_window_level level, enum cgl_window_gl_profile gl_profile, bool v_sync)
{
    bool result = false;
    CGContextRef context;
    CGSRegionRef region;
    CGRect rect;

    window->connection = CGSMainConnectionID();
    if(!window->connection) {
        goto err;
    }

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->level = level;
    window->gl_profile = gl_profile;
    window->v_sync = v_sync ? 1 : 0;
    GetCurrentProcess(&window->psn);

    rect = CGRectMake(0, 0, window->width, window->height);
    CGSNewRegionWithRect(&rect, &region);
    if(!region) {
        goto err;
    }

    CGSNewWindow(window->connection, kCGSBackingBuffered, window->x, window->y, region, &window->id);
    if(!window->id) {
        goto err_region;
    }

    CGSAddActivationRegion(window->connection, window->id, region);
    CGSAddTrackingRect(window->connection, window->id, rect);

    CGSSetWindowOpacity(window->connection, window->id, 0);
    CGSSetWindowLevel(window->connection, window->id, CGWindowLevelForKey((CGWindowLevelKey)window->level));

    TransformProcessType(&window->psn, kProcessTransformToForegroundApplication);
    cgl_window_bring_to_front(window);

    context = CGWindowContextCreate(window->connection, window->id, 0);
    CGContextClearRect(context, rect);
    CGContextRelease(context);

    result = cgl_window_context_init(window);

err_region:
    CFRelease(region);

err:
    return result;
}
#pragma clang diagnostic pop

bool cgl_window_move(struct cgl_window *window, float x, float y)
{
   CGPoint window_pos = { .x = x, .y = y };

   if (CGSMoveWindow(window->connection, window->id, &window_pos) != kCGErrorSuccess) {
       return false;
   }

   window->x = x;
   window->y = y;
   return true;
}

bool cgl_window_resize(struct cgl_window *window, float width, float height)
{
    bool result = false;
    CGSRegionRef shape;
    CGRect rect = CGRectMake(0, 0, width, height);

    if (CGSNewRegionWithRect(&rect, &shape) != kCGErrorSuccess) {
        goto err;
    }

    if (CGSSetWindowShape(window->connection, window->id, window->x, window->y, shape) != kCGErrorSuccess) {
        goto err_region;
    }

    window->width = width;
    window->height = height;

    CGSClearActivationRegion(window->connection, window->id);
    CGSAddActivationRegion(window->connection, window->id, shape);
    CGSRemoveAllTrackingAreas(window->connection, window->id);
    CGSAddTrackingRect(window->connection, window->id, rect);

    if (window->surface) {
        CGSRemoveSurface(window->connection, window->id, window->surface);
    }
    cgl_window_surface_init(window);
    result = true;

err_region:
    CFRelease(shape);

err:
    return result;
}

void cgl_window_add_drag_region(struct cgl_window *window, float x, float y, float width, float height)
{
    CGSRegionRef region;
    CGRect rect = CGRectMake(x, y, width, height);
    CGSNewRegionWithRect(&rect, &region);
    CGSAddDragRegion(window->connection, window->id, region);
}

void cgl_window_clear_drag_region(struct cgl_window *window)
{
    CGSClearDragRegion(window->connection, window->id);
}

void cgl_window_set_sticky(struct cgl_window *window, bool sticky)
{
    int tags[2] = {0};
    tags[0] |= (1 << 11);
    if (sticky) {
        CGSSetWindowTags(window->connection, window->id, tags, 32);
    } else {
        CGSClearWindowTags(window->connection, window->id, tags, 32);
    }
}

void cgl_window_set_alpha(struct cgl_window *window, float alpha)
{
    CGSSetWindowAlpha(window->connection, window->id, alpha);
}

void cgl_window_set_level(struct cgl_window *window, enum cgl_window_level level)
{
    CGSSetWindowLevel(window->connection, window->id, CGWindowLevelForKey((CGWindowLevelKey)level));
}

void cgl_window_destroy(struct cgl_window *window)
{
    CGLDestroyContext(window->context);
    CGSReleaseWindow(window->connection, window->id);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
void cgl_window_bring_to_front(struct cgl_window *window)
{
    CGSOrderWindow(window->connection, window->id, kCGSOrderAbove, 0);
    SetFrontProcess(&window->psn);
}
#pragma clang diagnostic pop

void cgl_window_set_mouse_callback(struct cgl_window *window, cgl_window_event_callback *mouse_callback)
{
    window->mouse_callback = mouse_callback;
}

void cgl_window_set_key_callback(struct cgl_window *window, cgl_window_event_callback *key_callback)
{
    window->key_callback = key_callback;
}

void cgl_window_set_application_callback(struct cgl_window *window, cgl_window_event_callback *application_callback)
{
    window->application_callback = application_callback;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
void cgl_window_show_cursor(struct cgl_window *window, bool visible)
{
    ProcessSerialNumber front_psn;
    GetFrontProcess(&front_psn);

    if ((front_psn.highLongOfPSN == window->psn.highLongOfPSN) &&
        (front_psn.lowLongOfPSN == window->psn.lowLongOfPSN)) {
        if (visible) {
            while (!CGCursorIsVisible()) {
                CGDisplayShowCursor(0);
            }
        } else {
            while (CGCursorIsVisible()) {
                CGDisplayHideCursor(0);
            }
        }
    }
}
#pragma clang diagnostic pop

bool cgl_window_toggle_fullscreen(struct cgl_window *window)
{
    static CGRect window_rect = {};
    if (window_rect.size.width == 0) {
        window_rect.origin.x = window->x;
        window_rect.origin.y = window->y;
        window_rect.size.width = window->width;
        window_rect.size.height = window->height;
        CFStringRef display_ref = CGSCopyManagedDisplayForWindow(window->connection, window->id);
        CFUUIDRef display_uuid = CFUUIDCreateFromString(NULL, display_ref);
        CGDirectDisplayID display_id = CGDisplayGetDisplayIDFromUUID(display_uuid);
        CFRelease(display_uuid);
        CFRelease(display_ref);
        CGRect display_rect = CGDisplayBounds(display_id);
        cgl_window_move(window, display_rect.origin.x, display_rect.origin.y);
        cgl_window_resize(window, display_rect.size.width, display_rect.size.height);
        return true;
    } else {
        cgl_window_move(window, window_rect.origin.x, window_rect.origin.y);
        cgl_window_resize(window, window_rect.size.width, window_rect.size.height);
        window_rect.size.width = 0;
        return false;
    }
}

void cgl_window_make_current(struct cgl_window *window)
{
    CGLSetCurrentContext(window->context);
}

CGLError cgl_window_flush(struct cgl_window *window)
{
    return CGLFlushDrawable(window->context);
}

#if 0
static void
debug_print_event_class(OSType event_class)
{
    switch (event_class) {
    case kEventClassMouse:          { printf("kEventClassMouse:%d\t", event_class);         } break;
    case kEventClassKeyboard:       { printf("kEventClassKeyboard:%d\t", event_class);      } break;
    case kEventClassTextInput:      { printf("kEventClassTextInput:%d\t", event_class);     } break;
    case kEventClassApplication:    { printf("kEventClassApplication:%d\t", event_class);   } break;
    case kEventClassAppleEvent:     { printf("kEventClassAppleEvent:%d\t", event_class);    } break;
    case kEventClassMenu:           { printf("kEventClassMenu:%d\t", event_class);          } break;
    case kEventClassWindow:         { printf("kEventClassWindow:%d\t", event_class);        } break;
    case kEventClassControl:        { printf("kEventClassControl:%d\t", event_class);       } break;
    case kEventClassCommand:        { printf("kEventClassCommand:%d\t", event_class);       } break;
    case kEventClassTablet:         { printf("kEventClassTablet:%d\t", event_class);        } break;
    case kEventClassVolume:         { printf("kEventClassVolume:%d\t", event_class);        } break;
    case kEventClassAppearance:     { printf("kEventClassAppearance:%d\t", event_class);    } break;
    case kEventClassService:        { printf("kEventClassService:%d\t", event_class);       } break;
    case kEventClassToolbar:        { printf("kEventClassToolbar:%d\t", event_class);       } break;
    case kEventClassToolbarItem:    { printf("kEventClassToolbarItem:%d\t", event_class);   } break;
    case kEventClassAccessibility:  { printf("kEventClassAccessibility:%d\t", event_class); } break;
    default:                        { printf("event class unknown:%d\t", event_class);      } break;
    }
}
#endif

/*
 * NOTE(koekeishiya): this event-class is not amongs the constants defined above for some reason,
 * however it appears to report window-related events.
 */
#define kUnknownEventClassWindow 1667724064
#define kUnknownEventWindowMoved 13

void cgl_window_poll_events(struct cgl_window *window, void *user_data)
{
    EventTargetRef event_target = GetEventDispatcherTarget();
    EventRef event;

    while (ReceiveNextEvent(0, 0, kEventDurationNoWait, true, &event) == noErr) {
        OSType event_class = GetEventClass(event);

        if (event_class == kEventClassMouse) {
            if (window->mouse_callback) {
                window->mouse_callback(window, event, user_data);
            }
        } else if (event_class == kEventClassKeyboard) {
            if (window->key_callback) {
                window->key_callback(window, event, user_data);
            }
        } else if (event_class == kEventClassApplication) {
            if (window->application_callback) {
                window->application_callback(window, event, user_data);
            }
        } else if (event_class == kUnknownEventClassWindow) {
            uint32_t event_kind = GetEventKind(event);
            if (event_kind == kUnknownEventWindowMoved) {
                CGRect rect;
                CGSGetScreenRectForWindow(window->connection, window->id, &rect);
                window->x = rect.origin.x;
                window->y = rect.origin.y;
            }
        } else if (event_class == kEventClassAppleEvent) {
            AEProcessEvent(event);
        }

        SendEventToEventTarget(event, event_target);
        ReleaseEvent(event);
    }
}
