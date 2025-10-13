//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "../include/brx_wsi.h"
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <QuartzCore/CAMetalLayer.h>
#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include "../../Brioche-ImGui/imgui.h"
#include "../../Brioche-ImGui/backends/imgui_impl_osx.h"
#include "../../McRT-Malloc/include/mcrt_vector.h"
#include "../../McRT-Malloc/include/mcrt_string.h"

@interface internal_brx_wsi_application_delegate : NSObject <NSApplicationDelegate>
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender;
- (void)applicationWillTerminate:(NSNotification *)notification;
@end

@interface internal_brx_wsi_main_view : NSView <NSTextInputClient>
- (void)set_platform_ime_data:(ImGuiPlatformImeData *)data __attribute__((objc_direct));
@end

@interface internal_brx_wsi_image_view : NSView
- (void)put_image:(void const *)image_data_base width:(int)image_width height:(int)image_height __attribute__((objc_direct));
@end

static NSAutoreleasePool *s_auto_release_pool = nil;
static NSApplication *s_shared_application = nil;
static internal_brx_wsi_application_delegate *s_application_delegate = nil;
static bool s_running = false;

extern "C" void brx_wsi_init_connection()
{
    assert(nil == s_auto_release_pool);
    s_auto_release_pool = [[NSAutoreleasePool alloc] init];

    assert(CFRunLoopGetMain() == CFRunLoopGetCurrent());

    NSAutoreleasePool *auto_release_pool_init_connection = [[NSAutoreleasePool alloc] init];

    assert(nil == s_shared_application);
    s_shared_application = [NSApplication sharedApplication];

#if 0
    [s_shared_application setActivationPolicy:NSApplicationActivationPolicyRegular];

    {
        constexpr size_t const icon_row_size = sizeof(uint32_t) * ICON_OPENCV_WIDTH;
        constexpr size_t const icon_data_size = icon_row_size * ICON_OPENCV_HEIGHT;
        static_assert(sizeof(ICON_OPENCV_DATA) == icon_data_size, "");

        CGColorSpaceRef icon_color_space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        assert(NULL != icon_color_space);

        CGDataProviderRef icon_data_provider = CGDataProviderCreateWithData(NULL, ICON_OPENCV_DATA, icon_data_size, NULL);
        assert(NULL != icon_data_provider);

        CGImageRef icon_cg_image = CGImageCreate(ICON_OPENCV_WIDTH, ICON_OPENCV_HEIGHT, 8, 32, icon_row_size, icon_color_space, kCGBitmapByteOrder32Little | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst, icon_data_provider, NULL, false, kCGRenderingIntentDefault);
        assert(NULL != icon_cg_image);

        NSImage *icon_ns_image = [[NSImage alloc] initWithCGImage:icon_cg_image size:NSMakeSize((CGFloat)ICON_OPENCV_WIDTH, (CGFloat)ICON_OPENCV_HEIGHT)];
        assert(NULL != icon_ns_image);

        CGImageRelease(icon_cg_image);
        icon_cg_image = NULL;

        CGDataProviderRelease(icon_data_provider);
        icon_data_provider = NULL;

        CGColorSpaceRelease(icon_color_space);
        icon_color_space = NULL;

        [s_shared_application setApplicationIconImage:icon_ns_image];

        [icon_ns_image release];
        icon_ns_image = nil;
    }
#endif

    assert(nil == s_application_delegate);
    s_application_delegate = [[internal_brx_wsi_application_delegate alloc] init];

    [s_shared_application setDelegate:s_application_delegate];

    [s_shared_application finishLaunching];

    [auto_release_pool_init_connection release];
    auto_release_pool_init_connection = nil;
}

extern "C" void brx_wsi_uninit_connection()
{
    assert(CFRunLoopGetMain() == CFRunLoopGetCurrent());

    assert(nil != s_application_delegate);
    [s_application_delegate release];
    s_application_delegate = nil;

    [s_auto_release_pool release];
    s_auto_release_pool = nil;
}

extern "C" void *brx_wsi_get_connection()
{
    return NULL;
}

static internal_brx_wsi_main_view *s_main_window_view = nil;
static int32_t s_main_window_width = 0;
static int32_t s_main_window_height = 0;
static float s_main_window_width_scale = 0.0F;
static float s_main_window_height_scale = 0.0F;

static void (*s_main_window_key_press_handler)(void *handler_context, int key, bool shift_key, bool caps_key, bool ctrl_key, bool alt_key) = NULL;
static void (*s_main_window_key_release_handler)(void *handler_context, int key, bool shift_key, bool caps_key, bool ctrl_key, bool alt_key) = NULL;
static void (*s_main_window_button_press_handler)(void *handler_context, int button, int x, int y) = NULL;
static void (*s_main_window_button_release_handler)(void *handler_context, int button, int x, int y) = NULL;
static void (*s_main_window_scroll_up_handler)(void *handler_context, int x, int y) = NULL;
static void (*s_main_window_scroll_down_handler)(void *handler_context, int x, int y) = NULL;
static void (*s_main_window_motion_handler)(void *handler_context, int x, int y, bool left_button, bool middle_button, bool right_button) = NULL;
static void (*s_main_window_resize_handler)(void *handler_context, int width, int height, float width_scale, float height_scale) = NULL;
static void *s_main_window_handler_context = NULL;

extern "C" void brx_wsi_init_main_window(
    char const *window_name,
    void (*key_press_handler)(void *handler_context, int key, bool shift_key, bool caps_key, bool ctrl_key, bool alt_key),
    void (*key_release_handler)(void *handler_context, int key, bool shift_key, bool caps_key, bool ctrl_key, bool alt_key),
    void (*button_press_handler)(void *handler_context, int button, int x, int y),
    void (*button_release_handler)(void *handler_context, int button, int x, int y),
    void (*scroll_up_handler)(void *handler_context, int x, int y),
    void (*scroll_down_handler)(void *handler_context, int x, int y),
    void (*motion_handler)(void *handler_context, int x, int y, bool left_button, bool middle_button, bool right_button),
    void (*resize_handler)(void *handler_context, int width, int height, float width_scale, float height_scale),
    int window_width_integer,
    int window_height_integer,
    void *handler_context)
{
    assert(CFRunLoopGetMain() == CFRunLoopGetCurrent());

    NSAutoreleasePool *auto_release_pool_init_main_window = [[NSAutoreleasePool alloc] init];

    assert(NULL == s_main_window_key_press_handler);
    s_main_window_key_press_handler = key_press_handler;
    assert(NULL != s_main_window_key_press_handler);

    assert(NULL == s_main_window_key_release_handler);
    s_main_window_key_release_handler = key_release_handler;
    assert(NULL != s_main_window_key_release_handler);

    assert(NULL == s_main_window_button_press_handler);
    s_main_window_button_press_handler = button_press_handler;
    assert(NULL != s_main_window_button_press_handler);

    assert(NULL == s_main_window_button_release_handler);
    s_main_window_button_release_handler = button_release_handler;
    assert(NULL != s_main_window_button_release_handler);

    assert(NULL == s_main_window_scroll_up_handler);
    s_main_window_scroll_up_handler = scroll_up_handler;
    assert(NULL != s_main_window_scroll_up_handler);

    assert(NULL == s_main_window_scroll_down_handler);
    s_main_window_scroll_down_handler = scroll_down_handler;
    assert(NULL != s_main_window_scroll_down_handler);

    assert(NULL == s_main_window_motion_handler);
    s_main_window_motion_handler = motion_handler;
    assert(NULL != s_main_window_motion_handler);

    assert(NULL == s_main_window_resize_handler);
    s_main_window_resize_handler = resize_handler;
    assert(NULL != s_main_window_resize_handler);

    assert(NULL == s_main_window_handler_context);
    s_main_window_handler_context = handler_context;

    {
        assert(nil == s_main_window_view);

        CGFloat window_width = window_width_integer;
        CGFloat window_height = window_height_integer;

        NSRect main_screen_rect = [[NSScreen mainScreen] visibleFrame];

        NSRect window_rect = NSMakeRect((main_screen_rect.size.width - window_width) * 0.5F + main_screen_rect.origin.x, (main_screen_rect.size.height - window_height) * 0.5F + main_screen_rect.origin.y, window_width, window_height);

        NSWindow *window = [[NSWindow alloc] initWithContentRect:window_rect styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:FALSE];
        assert(nil != window);

        [window setAcceptsMouseMovedEvents:YES];

        NSString *title = [[NSString alloc] initWithUTF8String:window_name];
        [window setTitle:title];
        [title release];

        s_main_window_view = [[internal_brx_wsi_main_view alloc] initWithFrame:NSMakeRect(0, 0, window_width, window_height)];

        [s_main_window_view setWantsLayer:YES];

        [s_main_window_view setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawDuringViewResize];

        [window setContentView:s_main_window_view];
        // view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

        // [window makeKeyAndOrderFront:nil];

        // since NSWindow releasedWhenClosed == YES by default
        [window setReleasedWhenClosed:YES];
        // we do not need to release the reference of window
        // [window release];

        window = nil;

        assert(nil != s_main_window_view);
    }

    assert(0 == s_main_window_width);
    s_main_window_width = window_width_integer;
    assert(0 != s_main_window_width);

    assert(0 == s_main_window_height);
    s_main_window_height = window_height_integer;
    assert(0 != s_main_window_height);

    CGSize const view_scale = [s_main_window_view convertSizeToBacking:CGSizeMake(1.0F, 1.0F)];
#ifndef NDEBUG
    {
        CALayer *layer = [s_main_window_view layer];
        assert(nil != layer);

        float const layer_scale = [layer contentsScale];
        assert(layer_scale == view_scale.width);
        assert(layer_scale == view_scale.height);

        NSWindow *window = [s_main_window_view window];
        assert(nil != window);

        float const window_scale = [window backingScaleFactor];
        assert(window_scale == view_scale.width);
        assert(window_scale == view_scale.height);
    }
#endif
    assert(0.0F == s_main_window_width_scale);
    s_main_window_width_scale = view_scale.width;
    assert(0.0F != s_main_window_width_scale);

    assert(0.0F == s_main_window_height_scale);
    s_main_window_height_scale = view_scale.height;
    assert(0.0F != s_main_window_height_scale);

    [auto_release_pool_init_main_window release];
    auto_release_pool_init_main_window = nil;
}

extern "C" void brx_wsi_uninit_main_window()
{
    assert(CFRunLoopGetMain() == CFRunLoopGetCurrent());

    assert(nil != s_main_window_view);
    NSWindow *window = [s_main_window_view window];
    if (nil != window)
    {
        [window setContentView:nil];
        [window close];
    }
    else
    {
        assert(false);
    }

    [s_main_window_view release];
    s_main_window_view = nil;

    s_main_window_handler_context = NULL;

    assert(NULL != s_main_window_key_press_handler);
    s_main_window_key_press_handler = NULL;

    assert(NULL != s_main_window_key_release_handler);
    s_main_window_key_release_handler = NULL;

    assert(NULL != s_main_window_button_press_handler);
    s_main_window_button_press_handler = NULL;

    assert(NULL != s_main_window_button_release_handler);
    s_main_window_button_release_handler = NULL;

    assert(NULL != s_main_window_scroll_up_handler);
    s_main_window_scroll_up_handler = NULL;

    assert(NULL != s_main_window_scroll_down_handler);
    s_main_window_scroll_down_handler = NULL;

    assert(NULL != s_main_window_motion_handler);
    s_main_window_motion_handler = NULL;

    assert(NULL != s_main_window_resize_handler);
    s_main_window_resize_handler = NULL;
}

extern "C" void *brx_wsi_get_main_window()
{
    assert(nil != s_main_window_view);
    return s_main_window_view;
}

extern "C" void brx_wsi_get_main_window_size(int *out_window_width, int *out_window_height)
{
    (*out_window_width) = s_main_window_width;
    (*out_window_height) = s_main_window_height;
}

extern "C" void brx_wsi_get_main_window_scale(float *out_window_width_scale, float *out_window_height_scale)
{
    (*out_window_width_scale) = s_main_window_width_scale;
    (*out_window_height_scale) = s_main_window_height_scale;
}

extern "C" void brx_wsi_show_main_window()
{
    assert(CFRunLoopGetMain() == CFRunLoopGetCurrent());

    NSAutoreleasePool *auto_release_pool_show_main_window = [[NSAutoreleasePool alloc] init];

    assert(nil != s_main_window_view);
    NSWindow *window = [s_main_window_view window];
    assert(nil != window);

    [window makeFirstResponder:s_main_window_view];

    [window makeKeyAndOrderFront:nil];

    [auto_release_pool_show_main_window release];
    auto_release_pool_show_main_window = nil;
}

extern "C" void brx_wsi_set_main_window_size(int window_width, int window_height)
{
    assert(CFRunLoopGetMain() == CFRunLoopGetCurrent());

    NSAutoreleasePool *auto_release_pool_set_main_window_size = [[NSAutoreleasePool alloc] init];

    assert(nil != s_main_window_view);
    NSWindow *window = [s_main_window_view window];
    if (nil != window)
    {
        [window setContentSize:NSMakeSize((CGFloat)window_width, (CGFloat)window_height)];
    }
    else
    {
        assert(false);
    }

    [auto_release_pool_set_main_window_size release];
    auto_release_pool_set_main_window_size = nil;
}

extern "C" void *brx_wsi_create_image_window(char const *window_name)
{
    assert(CFRunLoopGetMain() == CFRunLoopGetCurrent());

    NSAutoreleasePool *auto_release_pool_create_image_window = [[NSAutoreleasePool alloc] init];

    internal_brx_wsi_image_view *view = nil;
    {
        CGFloat window_width = 256.0F;
        CGFloat window_height = 256.0F;

        NSRect main_screen_rect = [[NSScreen mainScreen] visibleFrame];

        NSRect window_rect = NSMakeRect((main_screen_rect.size.width - window_width) * 0.5F + main_screen_rect.origin.x, (main_screen_rect.size.height - window_height) * 0.5F + main_screen_rect.origin.y, window_width, window_height);

        NSWindow *window = [[NSWindow alloc] initWithContentRect:window_rect styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:FALSE];
        assert(nil != window);

        NSString *title = [[NSString alloc] initWithUTF8String:window_name];
        [window setTitle:title];
        [title release];

        assert(nil == view);
        view = [[internal_brx_wsi_image_view alloc] initWithFrame:NSMakeRect(0, 0, window_width, window_height)];
        assert(nil != view);

        [window setContentView:view];

        [window makeKeyAndOrderFront:nil];

        // since NSWindow releasedWhenClosed == YES by default
        [window setReleasedWhenClosed:YES];
        // we do not need to release the reference of window
        // [window release];

        window = nil;
    }

    [auto_release_pool_create_image_window release];
    auto_release_pool_create_image_window = nil;

    return view;
}

extern "C" void brx_wsi_destroy_image_window(void *wrapped_window)
{
    assert(CFRunLoopGetMain() == CFRunLoopGetCurrent());

    internal_brx_wsi_image_view *view = (internal_brx_wsi_image_view *)wrapped_window;
    assert(nil != view);
    assert(NO != [view isKindOfClass:[internal_brx_wsi_image_view class]]);

    NSWindow *window = [view window];
    if (nil != window)
    {
        [window setContentView:nil];
        [window close];
    }
    else
    {
        assert(false);
    }

    [view release];
}

extern "C" void brx_wsi_present_image_window(void *wrapped_window, void const *image_buffer, int image_width, int image_height)
{
    assert(CFRunLoopGetMain() == CFRunLoopGetCurrent());

    NSAutoreleasePool *auto_release_pool_present_image_window = [[NSAutoreleasePool alloc] init];

    internal_brx_wsi_image_view *view = (internal_brx_wsi_image_view *)wrapped_window;
    assert(nil != view);
    assert(NO != [view isKindOfClass:[internal_brx_wsi_image_view class]]);

    if (nil != [view window])
    {
        [view put_image:image_buffer width:image_width height:image_height];
    }
    else
    {
        assert(false);
    }

    [auto_release_pool_present_image_window release];
    auto_release_pool_present_image_window = nil;
}

extern "C" void brx_wsi_run_main_loop(bool (*tick_callback)(void *tick_context), void *tick_context)
{
    assert(CFRunLoopGetMain() == CFRunLoopGetCurrent());

    NSAutoreleasePool *auto_release_pool_run_main_loop = [[NSAutoreleasePool alloc] init];

    struct internal_run_loop_observer_info_t
    {
        bool (*const m_tick_callback)(void *tick_context);
        void *const m_tick_context;
    } const internal_run_loop_observer_info = {tick_callback, tick_context};

    CFRunLoopObserverRef run_loop_observer = NULL;
    {
        assert(NULL == run_loop_observer);
        CFRunLoopObserverContext run_loop_observer_context = {0, const_cast<internal_run_loop_observer_info_t *>(&internal_run_loop_observer_info), NULL, NULL, NULL};
        run_loop_observer = CFRunLoopObserverCreate(
            kCFAllocatorDefault, kCFRunLoopBeforeWaiting,
            true,
            0,
            [](CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *wrapped_info) -> void
            {
                assert(NULL != wrapped_info);
                internal_run_loop_observer_info_t const *const internal_run_loop_observer_info = static_cast<internal_run_loop_observer_info_t *>(wrapped_info);
                bool (*const tick_callback)(void *tick_context) = internal_run_loop_observer_info->m_tick_callback;
                void *const tick_context = internal_run_loop_observer_info->m_tick_context;

                NSWindow *main_window = [s_main_window_view window];
                assert(nil != main_window);

                if (NO != [[s_shared_application windows] containsObject:main_window])
                {
                    if (s_running)
                    {
                        NSAutoreleasePool *auto_release_pool_tick_callback = [[NSAutoreleasePool alloc] init];

                        NSRect const view_bounds = [s_main_window_view bounds];

                        CGSize const view_scale = [s_main_window_view convertSizeToBacking:CGSizeMake(1.0F, 1.0F)];
#ifndef NDEBUG
                        {
                            CALayer *layer = [s_main_window_view layer];
                            assert(nil != layer);

                            float const layer_scale = [layer contentsScale];
                            assert(layer_scale == view_scale.width);
                            assert(layer_scale == view_scale.height);

                            float const window_scale = [main_window backingScaleFactor];
                            assert(window_scale == view_scale.width);
                            assert(window_scale == view_scale.height);
                        }
#endif

                        if ((view_bounds.size.width != s_main_window_width) || (view_bounds.size.height != s_main_window_height) || (std::abs(view_scale.width - s_main_window_width_scale) > 1E-3F) || (std::abs(view_scale.height - s_main_window_height_scale) > 1E-3F))
                        {
                            s_main_window_width = view_bounds.size.width;
                            s_main_window_height = view_bounds.size.height;

                            s_main_window_width_scale = view_scale.width;
                            s_main_window_height_scale = view_scale.height;

                            ImGui_ImplOSX_ReshapeFunc(s_main_window_width, s_main_window_height, s_main_window_width_scale, s_main_window_height_scale);

                            s_main_window_resize_handler(s_main_window_handler_context, s_main_window_width, s_main_window_height, s_main_window_width_scale, s_main_window_height_scale);
                        }

                        s_running = tick_callback(tick_context);

                        [auto_release_pool_tick_callback release];
                        auto_release_pool_tick_callback = nil;

                        if (__builtin_expect(!!(!s_running), 0))
                        {
                            [s_shared_application stop:nil];
                        }
                    }
                }
                else
                {
                    s_running = false;
                    [s_shared_application stop:nil];
                }

                CFRunLoopWakeUp(CFRunLoopGetMain());
            },
            &run_loop_observer_context);
        CFRunLoopAddObserver(CFRunLoopGetMain(), run_loop_observer, kCFRunLoopCommonModes);
    }

    assert(false == s_running);
    s_running = true;

    [s_shared_application run];

    assert(false == s_running);

    assert(NULL != run_loop_observer);
    CFRunLoopRemoveObserver(CFRunLoopGetMain(), run_loop_observer, kCFRunLoopCommonModes);
    CFRelease(run_loop_observer);
    run_loop_observer = NULL;

    [auto_release_pool_run_main_loop release];
    auto_release_pool_run_main_loop = nil;
}

@implementation internal_brx_wsi_application_delegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    // we use "stop" instead of "terminate" to allow return from "run"
    s_running = false;
    [sender stop:nil];
    return NSTerminateCancel;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    // we use "stop" instead of "terminate" to allow return from "run"
    s_running = false;
    [sender stop:nil];
    return NO;
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    // we use "stop" instead of "terminate" to allow return from "run"
    assert(false);
    s_running = false;
}
@end

extern "C" IMGUI_IMPL_API ImGuiKey ImGui_ImplOSX_KeyCodeToImGuiKey(unsigned short key_code);

@implementation internal_brx_wsi_main_view
{
    float _platform_ime_data_input_pos_x;
    float _platform_ime_data_input_pos_y;
    float _platform_ime_data_input_line_height;
}

// MoltenVK

// [DemoView](https://github.com/KhronosGroup/MoltenVK/blob/main/Demos/Cube/macOS/DemoViewController.m#L117)

// Indicates that the view wants to draw using the backing layer instead of using drawRect:.
- (BOOL)wantsUpdateLayer
{
    return YES;
}

// Returns a Metal-compatible layer.
+ (Class)layerClass
{
    return [CAMetalLayer class];
}

// If the wantsLayer property is set to YES, this method will be invoked to return a layer instance.
- (CALayer *)makeBackingLayer
{
    CALayer *layer = [self.class.layerClass layer];
    CGSize view_scale = [self convertSizeToBacking:CGSizeMake(1.0, 1.0)];
    layer.contentsScale = std::min(view_scale.width, view_scale.height);
    return layer;
}

// If this view moves to a screen that has a different resolution scale (eg. Standard <=> Retina), update the contentsScale of the layer, which will trigger a Vulkan VK_SUBOPTIMAL_KHR result, which causes this demo to replace the swapchain, in order to optimize rendering for the new resolution.
- (BOOL)layer:(CALayer *)layer shouldInheritContentsScale:(CGFloat)newScale fromWindow:(NSWindow *)window
{
    if (newScale == layer.contentsScale)
    {
        return NO;
    }

    layer.contentsScale = newScale;
    return YES;
}

// User Input

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (NSPoint)internal_mouse_position:(NSEvent *)event __attribute__((objc_direct))
{
    NSPoint point_in_window;
    if (nil != [event window])
    {
        point_in_window = [event locationInWindow];
    }
    else
    {
        NSPoint point_in_screen = [event locationInWindow];

        NSWindow *view_window = [self window];
        assert(nil != view_window);

        point_in_window = [view_window convertPointFromScreen:point_in_screen];
    }

    NSPoint point_in_view = [self convertPoint:point_in_window fromView:nil];

    NSPoint point_in_view_flip;
    if (NO == [self isFlipped])
    {
        NSRect view_bounds = [self bounds];
        point_in_view_flip = NSMakePoint(point_in_view.x, view_bounds.size.height - point_in_view.y);
    }
    else
    {
        point_in_view_flip = point_in_view;
    }

    return point_in_view_flip;
}

- (void)mouseDown:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);

    NSPoint mouse_position = [self internal_mouse_position:event];

    s_main_window_button_press_handler(s_main_window_handler_context, ImGuiMouseButton_Left, mouse_position.x, mouse_position.y);
}

- (void)rightMouseDown:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);

    NSPoint mouse_position = [self internal_mouse_position:event];

    s_main_window_button_press_handler(s_main_window_handler_context, ImGuiMouseButton_Right, mouse_position.x, mouse_position.y);
}

- (void)otherMouseDown:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);

    NSPoint mouse_position = [self internal_mouse_position:event];

    s_main_window_button_press_handler(s_main_window_handler_context, ImGuiMouseButton_Middle, mouse_position.x, mouse_position.y);
}

- (void)mouseUp:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);

    NSPoint mouse_position = [self internal_mouse_position:event];

    s_main_window_button_release_handler(s_main_window_handler_context, ImGuiMouseButton_Left, mouse_position.x, mouse_position.y);
}

- (void)rightMouseUp:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);

    NSPoint mouse_position = [self internal_mouse_position:event];

    s_main_window_button_release_handler(s_main_window_handler_context, ImGuiMouseButton_Right, mouse_position.x, mouse_position.y);
}

- (void)otherMouseUp:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);

    NSPoint mouse_position = [self internal_mouse_position:event];

    s_main_window_button_release_handler(s_main_window_handler_context, ImGuiMouseButton_Middle, mouse_position.x, mouse_position.y);
}

- (void)mouseMoved:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);

    NSPoint mouse_position = [self internal_mouse_position:event];

    s_main_window_motion_handler(s_main_window_handler_context, mouse_position.x, mouse_position.y, false, false, false);
}

- (void)mouseDragged:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);

    NSPoint mouse_position = [self internal_mouse_position:event];

    s_main_window_motion_handler(s_main_window_handler_context, mouse_position.x, mouse_position.y, true, false, false);
}

- (void)rightMouseDragged:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);

    NSPoint mouse_position = [self internal_mouse_position:event];

    s_main_window_motion_handler(s_main_window_handler_context, mouse_position.x, mouse_position.y, false, false, true);
}

- (void)otherMouseDragged:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);

    NSPoint mouse_position = [self internal_mouse_position:event];

    s_main_window_motion_handler(s_main_window_handler_context, mouse_position.x, mouse_position.y, false, true, false);
}

- (void)scrollWheel:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);

    double wheel_dy = 0.0;

    if (@available(macOS 10.7, *))
    {
        wheel_dy = [event scrollingDeltaY];
        if ([event hasPreciseScrollingDeltas])
        {
            wheel_dy *= 0.01;
        }
    }
    else
    {
        wheel_dy = [event deltaY] * 0.1;
    }

    NSPoint mouse_position = [self internal_mouse_position:event];

    if (wheel_dy > 0.0)
    {
        s_main_window_scroll_up_handler(s_main_window_handler_context, mouse_position.x, mouse_position.y);
    }
    else
    {
        s_main_window_scroll_down_handler(s_main_window_handler_context, mouse_position.x, mouse_position.y);
    }
}

- (void)keyDown:(NSEvent *)event
{
    // https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/TextEditing/Tasks/TextViewTask.html
    NSTextInputContext *input_context = [self inputContext];
    assert(nil != input_context);

    [input_context handleEvent:event];

    ImGui_ImplOSX_HandleEvent(event, self);

    if (NO == [event isARepeat])
    {
        unsigned short const key_code = (int)[event keyCode];

        ImGuiKey key = ImGui_ImplOSX_KeyCodeToImGuiKey(key_code);

        NSEventModifierFlags const modifier_flags = [event modifierFlags];

        bool const shift_key = (0U != (modifier_flags & NSEventModifierFlagShift));
        bool const caps_key = (0U != (modifier_flags & NSEventModifierFlagCapsLock));
        bool const ctrl_key = (0U != (modifier_flags & NSEventModifierFlagControl));
        bool const alt_key = (0U != (modifier_flags & NSEventModifierFlagOption));

        s_main_window_key_press_handler(s_main_window_handler_context, key, shift_key, caps_key, ctrl_key, alt_key);
    }
}

- (void)keyUp:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);

    if (NO == [event isARepeat])
    {
        unsigned short const key_code = (int)[event keyCode];

        ImGuiKey key = ImGui_ImplOSX_KeyCodeToImGuiKey(key_code);

        NSEventModifierFlags const modifier_flags = [event modifierFlags];

        bool const shift_key = (0U != (modifier_flags & NSEventModifierFlagShift));
        bool const caps_key = (0U != (modifier_flags & NSEventModifierFlagCapsLock));
        bool const ctrl_key = (0U != (modifier_flags & NSEventModifierFlagControl));
        bool const alt_key = (0U != (modifier_flags & NSEventModifierFlagOption));

        s_main_window_key_release_handler(s_main_window_handler_context, key, shift_key, caps_key, ctrl_key, alt_key);
    }
}

- (void)flagsChanged:(NSEvent *)event
{
    ImGui_ImplOSX_HandleEvent(event, self);
}

// Text Input

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange
{
    NSString *characters = [string isKindOfClass:[NSAttributedString class]] ? [(NSAttributedString *)string string] : (NSString *)string;
    char const *utf8_chars = [characters UTF8String];

    ImGuiIO &io = ImGui::GetIO();
    io.AddInputCharactersUTF8(utf8_chars);
}

- (void)doCommandBySelector:(SEL)selector
{
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
}

- (void)unmarkText
{
}

- (NSRange)selectedRange
{
    return NSMakeRange(NSNotFound, 0);
}

- (NSRange)markedRange
{
    return NSMakeRange(NSNotFound, 0);
}

- (BOOL)hasMarkedText
{
    return NO;
}

- (nullable NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
    return nil;
}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText
{
    return @[];
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
    NSWindow *window = [self window];
    assert(nil != window);

    if (NULL != actualRange)
    {
        (*actualRange) = range;
    }

    NSRect rect_in_view = NSMakeRect(_platform_ime_data_input_pos_x, _platform_ime_data_input_pos_y + _platform_ime_data_input_line_height, 0.0, 0.0);
    if (NO == [self isFlipped])
    {
        NSRect view_bounds = [self bounds];
        rect_in_view.origin.y = view_bounds.size.height - rect_in_view.origin.y;
    }

    NSRect rect_in_window = [self convertRect:rect_in_view toView:nil];

    NSRect rect_in_screen = [window convertRectToScreen:rect_in_window];

    return rect_in_screen;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
    return 0;
}

// ImGui
- (void)set_platform_ime_data:(ImGuiPlatformImeData *)data __attribute__((objc_direct))
{
    NSTextInputContext *input_context = [self inputContext];
    assert(nil != input_context);

    if (data->WantVisible)
    {
        [input_context activate];
    }
    else
    {
        [input_context discardMarkedText];
        [input_context invalidateCharacterCoordinates];
        [input_context deactivate];
    }

    self->_platform_ime_data_input_pos_x = data->InputPos.x;
    self->_platform_ime_data_input_pos_y = data->InputPos.y;
    self->_platform_ime_data_input_line_height = data->InputLineHeight;
}

@end

extern "C" void ImGui_ImplOSX_Platform_SetImeData(ImGuiContext *, ImGuiViewport *viewport, ImGuiPlatformImeData *data)
{
    NSAutoreleasePool *auto_release_pool_platform_set_ime_data = [[NSAutoreleasePool alloc] init];

    assert(nil != s_main_window_view);
    [s_main_window_view set_platform_ime_data:data];

    [auto_release_pool_platform_set_ime_data release];
    auto_release_pool_platform_set_ime_data = nil;
}

@implementation internal_brx_wsi_image_view
{
    void *_image_data_base;
    int _image_width;
    int _image_height;
    CGImageRef _image;
#ifndef NDEBUG
    bool _image_lock;
#endif
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (nil != self)
    {
#ifndef NDEBUG
        assert(!self->_image_lock);
        self->_image_lock = true;
#endif

        [self attach_image:frame.size.width
                    height:frame.size.height];

#ifndef NDEBUG
        self->_image_lock = false;
#endif
    }
    return self;
}

- (void)dealloc
{
    [self detach_image];

    assert(0 == self->_image_width);

    assert(0 == self->_image_height);

    assert(NULL == self->_image_data_base);

    assert(NULL == self->_image);

    // ARC OFF
    [super dealloc];
}

- (void)drawRect:(NSRect)dirtyRect
{
#ifndef NDEBUG
    assert(!self->_image_lock);
    self->_image_lock = true;
#endif

    CGContextRef graphics_context = [[NSGraphicsContext currentContext] CGContext];

    CGContextDrawImage(graphics_context, CGRectMake(0, 0, self->_image_width, self->_image_height), self->_image);

#ifndef NDEBUG
    self->_image_lock = false;
#endif
}

- (void)put_image:(void const *)image_data_base width:(int)image_width height:(int)image_height
{
    NSAutoreleasePool *auto_release_pool_put_image = [[NSAutoreleasePool alloc] init];

#ifndef NDEBUG
    assert(!self->_image_lock);
    self->_image_lock = true;
#endif

    if (__builtin_expect(!!((_image_width != image_width) || (_image_height != image_height)), 0))
    {
        [self detach_image];
        [self attach_image:image_width height:image_height];

        NSWindow *window = [self window];
        if (nil != window)
        {
            [window setContentSize:NSMakeSize((CGFloat)image_width, (CGFloat)image_height)];
        }
        else
        {
            assert(false);
        }
    }

    size_t const image_row_size = sizeof(uint32_t) * self->_image_width;
    size_t const image_data_size = image_row_size * self->_image_height;

    std::memcpy(self->_image_data_base, image_data_base, image_data_size);

    [self setNeedsDisplay:YES];

#ifndef NDEBUG
    self->_image_lock = false;
#endif

    // [self displayIfNeeded];

    [auto_release_pool_put_image release];
    auto_release_pool_put_image = nil;
}

- (void)attach_image:(int)image_width height:(int)image_height __attribute__((objc_direct))
{
    assert(0 == self->_image_width);
    self->_image_width = image_width;

    assert(0 == self->_image_height);
    self->_image_height = image_height;

    size_t const image_row_size = sizeof(uint32_t) * self->_image_width;
    size_t const image_data_size = image_row_size * self->_image_height;

    assert(NULL == self->_image_data_base);
    self->_image_data_base = std::malloc(image_data_size);
    assert(NULL != self->_image_data_base);

    CGColorSpaceRef image_color_space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    assert(NULL != image_color_space);

    CGDataProviderRef image_data_provider = CGDataProviderCreateWithData(NULL, self->_image_data_base, image_data_size, NULL);
    assert(NULL != image_data_provider);

    assert(NULL == self->_image);
    self->_image = CGImageCreate(self->_image_width, self->_image_height, 8, 32, image_row_size, image_color_space, kCGBitmapByteOrder32Little | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst, image_data_provider, NULL, false, kCGRenderingIntentDefault);
    assert(NULL != self->_image);

    CGColorSpaceRelease(image_color_space);

    CGDataProviderRelease(image_data_provider);
}

- (void)detach_image __attribute__((objc_direct))
{
    assert(NULL != self->_image);
    CGImageRelease(self->_image);
    self->_image = NULL;

    assert(NULL != self->_image_data_base);
    std::free(self->_image_data_base);
    self->_image_data_base = NULL;

    self->_image_width = 0;

    self->_image_height = 0;
}
@end

extern "C" bool brx_wsi_get_open_file_name(int filter_count, char const *const *, char const *const *filter_specs, int *, void *out_file_name_std_string, void (*std_string_assign_callback)(void *out_file_name_std_string, char const *s))
{
    assert(CFRunLoopGetMain() == CFRunLoopGetCurrent());

    mcrt_string file_name;
    bool open_file = false;
    {
        mcrt_vector<mcrt_string> file_name_extensions;
        for (int filter_index = 0; filter_index < filter_count; ++filter_index)
        {
            char const *const filter_spec_begin = filter_specs[filter_index];
            assert(NULL != filter_spec_begin);
            char const *const filter_spec_end = std::strchr(filter_spec_begin, '\0');
            assert(NULL != filter_spec_end);
            size_t const filter_spec_size = filter_spec_end - filter_spec_begin;

            size_t current = 0U;
            while (current < filter_spec_size)
            {
                char const *next = static_cast<char const *>(std::memchr(filter_spec_begin + current, ';', filter_spec_size - current));
                size_t token_begin = current;
                size_t token_end = (NULL != next) ? (next - filter_spec_begin) : filter_spec_size;
                current = (NULL != next) ? ((next - filter_spec_begin) + 1U) : filter_spec_size;

                {
                    while ((token_begin < token_end) && ((' ' == filter_spec_begin[token_begin]) || ('\t' == filter_spec_begin[token_begin])))
                    {
                        ++token_begin;
                    }

                    while ((token_begin < token_end) && ((' ' == filter_spec_begin[token_end - 1U]) || ('\t' == filter_spec_begin[token_end - 1U])))
                    {
                        --token_end;
                    }
                }

                if ((token_begin + 1U) < token_end)
                {
                    if (('*' == filter_spec_begin[token_begin]) && ('.' == filter_spec_begin[token_begin + 1U]))
                    {
                        token_begin += 2U;

                        if (token_begin < token_end)
                        {
                            if (NULL == std::memchr(filter_spec_begin + token_begin, '*', token_end - token_begin))
                            {
                                // "*.ext"
                                file_name_extensions.push_back(mcrt_string(filter_spec_begin + token_begin, token_end - token_begin));
                            }
                            else
                            {
                                // "*.*"
                                assert(('*' == filter_spec_begin[token_begin]) && ((token_begin + 1U) == token_end));
                            }
                        }
                        else
                        {
                            assert(false);
                        }
                    }
                    else
                    {
                        assert(false);
                    }
                }
                else
                {
                    // "*"
                    assert(('*' == filter_spec_begin[token_begin]) && ((token_begin + 1U) == token_end));
                }
            }
        }

        NSAutoreleasePool *auto_release_pool_get_open_file_name = [[NSAutoreleasePool alloc] init];

        NSOpenPanel *open_panel = [NSOpenPanel openPanel];
        [open_panel setAllowsMultipleSelection:NO];
        [open_panel setCanChooseFiles:YES];
        [open_panel setCanChooseDirectories:NO];

        if (@available(macOS 11.0, *))
        {
            NSMutableArray<UTType *> *allowed_file_types = [NSMutableArray arrayWithCapacity:file_name_extensions.size()];
            for (mcrt_string const &file_name_extension : file_name_extensions)
            {
                NSString *file_name_extension_string = [NSString stringWithUTF8String:file_name_extension.c_str()];
                assert(nil != file_name_extension_string);

                UTType *allowed_file_type = [UTType typeWithFilenameExtension:file_name_extension_string conformingToType:UTTypeData];
                assert(nil != allowed_file_type);

                [allowed_file_types addObject:allowed_file_type];
            }
            [open_panel setAllowedContentTypes:allowed_file_types];
        }
        else
        {
            NSMutableArray<NSString *> *allowed_file_types = [NSMutableArray arrayWithCapacity:file_name_extensions.size()];
            for (mcrt_string const &file_name_extension : file_name_extensions)
            {
                NSString *allowed_file_type = [NSString stringWithUTF8String:file_name_extension.c_str()];
                assert(nil != allowed_file_type);

                [allowed_file_types addObject:allowed_file_type];
            }
            [open_panel setAllowedFileTypes:allowed_file_types];
        }

        NSInteger res_open_panel_run_modal = [open_panel runModal];
        if (NSModalResponseOK == res_open_panel_run_modal)
        {
            NSURL *url = [open_panel URL];
            if (nil != url)
            {
                NSString *path = [url path];
                if (nil != path)
                {
                    file_name.assign([path fileSystemRepresentation]);
                    open_file = true;
                }
            }
        }

        [auto_release_pool_get_open_file_name release];
        auto_release_pool_get_open_file_name = nil;
    }

    if (open_file)
    {
        std_string_assign_callback(out_file_name_std_string, file_name.c_str());
        return true;
    }
    else
    {
        return false;
    }
}

extern "C" void brx_wsi_tcc()
{
    NSAutoreleasePool *auto_release_pool_tcc = [[NSAutoreleasePool alloc] init];

    if (@available(macOS 13.0, *))
    {
        [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.settings.PrivacySecurity.extension?Privacy_FilesAndFolders"]];
    }
    else if (@available(macOS 10.6, *))
    {
        [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_FilesAndFolders"]];
    }
    else
    {
        // We can NOT be later than macOS 10.14 (TCC) and early than macOS 10.6 at the same time
        assert(false);
        if (CFRunLoopGetMain() == CFRunLoopGetCurrent())
        {
            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_FilesAndFolders"]];
        }
        else
        {
            CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopDefaultMode, ^{
              [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_FilesAndFolders"]];
            });
        }
    }

    [auto_release_pool_tcc release];
    auto_release_pool_tcc = nil;
}
