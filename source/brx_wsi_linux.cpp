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
#include <xcb/xcb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cinttypes>
#include <algorithm>
#include "../../Brioche-ImGui/imgui.h"
#include "../../Brioche-ImGui/backends/imgui_impl_glut.h"
#include "../../McRT-Malloc/include/mcrt_vector.h"
#include "../../McRT-Malloc/include/mcrt_string.h"
#include "../build-linux/resource.h"

static int internal_evdev_key_to_imgui_key(xcb_keycode_t evdev_key);

static int internal_xcb_button_to_imgui_button(xcb_button_t xcb_button);

constexpr uint8_t const k_depth = 32U;

static xcb_connection_t *s_connection = NULL;
static uint32_t s_maximum_request_length = 0U;
static xcb_screen_t *s_screen = NULL;
static xcb_visualid_t s_visual_id = XCB_NONE;
static xcb_colormap_t s_colormap = XCB_NONE;
static xcb_atom_t s_atom_wm_protocols = XCB_NONE;
static xcb_atom_t s_atom_wm_delete_window = XCB_NONE;
static xcb_pixmap_t s_icon_pixmap = XCB_NONE;

struct internal_brx_wsi_pal_bridge_connection
{
    xcb_connection_t *m_connection;
    xcb_visualid_t m_visual_id;
};
static internal_brx_wsi_pal_bridge_connection s_pal_bridge_connection = {NULL, XCB_NONE};

static inline void internal_brx_wsi_set_window_icon(xcb_connection_t *connection, xcb_window_t window, xcb_pixmap_t icon_pixmap);

static inline void internal_brx_wsi_set_window_size(xcb_connection_t *connection, xcb_window_t window, int32_t width, int32_t height);

extern "C" void brx_wsi_init_connection()
{
    {
        int screen_number;
        assert(NULL == s_connection);
        s_connection = xcb_connect(NULL, &screen_number);
        assert(0 == xcb_connection_has_error(s_connection));

        xcb_setup_t const *setup = xcb_get_setup(s_connection);

        int i = 0;
        for (xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup); screen_iterator.rem > 0; xcb_screen_next(&screen_iterator))
        {
            if (i == screen_number)
            {
                assert(NULL == s_screen);
                s_screen = screen_iterator.data;
                break;
            }
            ++i;
        }
    }
    assert(NULL != s_connection);
    assert(NULL != s_screen);

    // enable big requests
    xcb_prefetch_maximum_request_length(s_connection);

    assert(0U == s_maximum_request_length);
    s_maximum_request_length = xcb_get_maximum_request_length(s_connection);
    assert(s_maximum_request_length > 0U);

    {
        for (xcb_depth_iterator_t depth_iterator = xcb_screen_allowed_depths_iterator(s_screen); depth_iterator.rem > 0; xcb_depth_next(&depth_iterator))
        {
            if (k_depth == depth_iterator.data->depth)
            {
                for (xcb_visualtype_iterator_t visual_iterator = xcb_depth_visuals_iterator(depth_iterator.data); visual_iterator.rem > 0; xcb_visualtype_next(&visual_iterator))
                {
                    if ((XCB_VISUAL_CLASS_TRUE_COLOR == visual_iterator.data->_class) && (0XFF0000U == visual_iterator.data->red_mask) && (0XFF00U == visual_iterator.data->green_mask) && (0XFFU == visual_iterator.data->blue_mask))
                    {
                        assert(XCB_NONE == s_visual_id);
                        s_visual_id = visual_iterator.data->visual_id;
                        break;
                    }
                }
                break;
            }
        }
    }
    assert(XCB_NONE != s_visual_id);

    {
        assert(XCB_NONE == s_colormap);
        s_colormap = xcb_generate_id(s_connection);

        xcb_void_cookie_t cookie_create_colormap = xcb_create_colormap_checked(s_connection, XCB_COLORMAP_ALLOC_NONE, s_colormap, s_screen->root, s_visual_id);

        xcb_generic_error_t *error_create_colormap = xcb_request_check(s_connection, cookie_create_colormap);
        assert(NULL == error_create_colormap);
    }
    assert(XCB_NONE != s_colormap);

    {
        xcb_intern_atom_cookie_t cookie_wm_protocols = xcb_intern_atom(s_connection, 0, 12U, "WM_PROTOCOLS");

        xcb_generic_error_t *error_intern_atom_reply_wm_protocols;
        xcb_intern_atom_reply_t *reply_wm_protocols = xcb_intern_atom_reply(s_connection, cookie_wm_protocols, &error_intern_atom_reply_wm_protocols);
        assert(NULL == error_intern_atom_reply_wm_protocols);
        assert(XCB_NONE == s_atom_wm_protocols);
        s_atom_wm_protocols = reply_wm_protocols->atom;
        std::free(reply_wm_protocols);
    }
    assert(XCB_NONE != s_atom_wm_protocols);

    {
        xcb_intern_atom_cookie_t cookie_wm_delete_window = xcb_intern_atom(s_connection, 0, 16U, "WM_DELETE_WINDOW");

        xcb_generic_error_t *error_intern_atom_reply_wm_delete_window;
        xcb_intern_atom_reply_t *reply_wm_delete_window = xcb_intern_atom_reply(s_connection, cookie_wm_delete_window, &error_intern_atom_reply_wm_delete_window);
        assert(NULL == error_intern_atom_reply_wm_delete_window);
        assert(XCB_NONE == s_atom_wm_delete_window);
        s_atom_wm_delete_window = reply_wm_delete_window->atom;
        std::free(reply_wm_delete_window);
    }
    assert(XCB_NONE != s_atom_wm_delete_window);

    assert(NULL == s_pal_bridge_connection.m_connection);
    s_pal_bridge_connection.m_connection = s_connection;
    assert(NULL != s_pal_bridge_connection.m_connection);

    assert(XCB_NONE == s_pal_bridge_connection.m_visual_id);
    s_pal_bridge_connection.m_visual_id = s_visual_id;
    assert(XCB_NONE != s_pal_bridge_connection.m_visual_id);

    {
        assert(XCB_NONE == s_icon_pixmap);
        s_icon_pixmap = xcb_generate_id(s_connection);
        assert(XCB_NONE != s_icon_pixmap);

        xcb_void_cookie_t cookie_create_icon_pixmap = xcb_create_pixmap_checked(s_connection, k_depth, s_icon_pixmap, s_screen->root, ICON_BRX_PUPPET_WIDTH, ICON_BRX_PUPPET_HEIGHT);

        xcb_generic_error_t *error_create_icon_pixmap = xcb_request_check(s_connection, cookie_create_icon_pixmap);
        assert(NULL == error_create_icon_pixmap);
    }

    {
        xcb_gcontext_t icon_pixmap_graphics_context = XCB_NONE;
        {
            assert(XCB_NONE == icon_pixmap_graphics_context);
            icon_pixmap_graphics_context = xcb_generate_id(s_connection);
            assert(XCB_NONE != icon_pixmap_graphics_context);

            xcb_void_cookie_t cookie_create_graphics_context = xcb_create_gc_checked(s_connection, icon_pixmap_graphics_context, s_icon_pixmap, 0U, NULL);

            xcb_generic_error_t *error_create_graphics_context = xcb_request_check(s_connection, cookie_create_graphics_context);
            assert(NULL == error_create_graphics_context);
        }

        {
            xcb_put_image(s_connection, XCB_IMAGE_FORMAT_Z_PIXMAP, s_icon_pixmap, icon_pixmap_graphics_context, ICON_BRX_PUPPET_WIDTH, ICON_BRX_PUPPET_HEIGHT, 0, 0, 0, k_depth, sizeof(uint32_t) * ICON_BRX_PUPPET_WIDTH * ICON_BRX_PUPPET_HEIGHT, reinterpret_cast<uint8_t const *>(&ICON_BRX_PUPPET_DATA[0][0]));

            int result_flush = xcb_flush(s_connection);
            assert(result_flush > 0);
        }

        {
            assert(XCB_NONE != icon_pixmap_graphics_context);
            xcb_void_cookie_t cookie_free_graphics_context = xcb_free_gc_checked(s_connection, icon_pixmap_graphics_context);
            icon_pixmap_graphics_context = XCB_NONE;

            xcb_generic_error_t *error_free_graphics_context = xcb_request_check(s_connection, cookie_free_graphics_context);
            assert(NULL == error_free_graphics_context);
        }
    }
}

extern "C" void brx_wsi_uninit_connection()
{
    assert(NULL != s_pal_bridge_connection.m_connection);
    s_pal_bridge_connection.m_connection = NULL;

    assert(XCB_NONE != s_pal_bridge_connection.m_visual_id);
    s_pal_bridge_connection.m_visual_id = XCB_NONE;

    {
        assert(XCB_NONE != s_icon_pixmap);
        xcb_void_cookie_t cookie_free_icon_pixmap = xcb_free_pixmap_checked(s_connection, s_icon_pixmap);
        s_icon_pixmap = XCB_NONE;

        xcb_generic_error_t *error_free_icon_pixmap = xcb_request_check(s_connection, cookie_free_icon_pixmap);
        assert(NULL == error_free_icon_pixmap);
    }

    assert(XCB_NONE != s_atom_wm_protocols);
    s_atom_wm_protocols = XCB_NONE;

    assert(XCB_NONE != s_atom_wm_delete_window);
    s_atom_wm_delete_window = XCB_NONE;

    {
        assert(XCB_NONE != s_colormap);
        xcb_void_cookie_t cookie_free_colormap = xcb_free_colormap_checked(s_connection, s_colormap);
        s_colormap = XCB_NONE;

        xcb_generic_error_t *error_free_colormap = xcb_request_check(s_connection, cookie_free_colormap);
        assert(NULL == error_free_colormap);
    }

    {
        assert(XCB_NONE != s_visual_id);
        s_visual_id = XCB_NONE;
    }

    {
        assert(NULL != s_screen);
        s_screen = NULL;
    }

    {
        assert(NULL != s_connection);
        xcb_disconnect(s_connection);
        s_connection = NULL;
    }
}

extern "C" void *brx_wsi_get_connection()
{
    return &s_pal_bridge_connection;
}

static xcb_window_t s_main_window = XCB_NONE;
static int32_t s_main_window_width = 0;
static int32_t s_main_window_height = 0;

struct internal_brx_wsi_pal_bridge_main_window
{
    xcb_connection_t *m_connection;
    xcb_window_t m_window;
};
static internal_brx_wsi_pal_bridge_main_window s_pal_bridge_main_window = {NULL, XCB_NONE};

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
    int window_width,
    int window_height,
    void *handler_context)
{
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
        assert(XCB_NONE == s_main_window);
        s_main_window = xcb_generate_id(s_connection);
        assert(XCB_NONE != s_main_window);

        // Both "border pixel" and "colormap" are required when the depth is NOT equal to the root window's.
        uint32_t value_mask = XCB_CW_BORDER_PIXEL | XCB_CW_BACKING_STORE | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;

        uint32_t value_list[4] = {0, XCB_BACKING_STORE_NOT_USEFUL, XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY, s_colormap};

        xcb_void_cookie_t cookie_create_window = xcb_create_window_checked(s_connection, k_depth, s_main_window, s_screen->root, 0, 0, window_width, window_height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, s_visual_id, value_mask, value_list);

        xcb_generic_error_t *error_create_window = xcb_request_check(s_connection, cookie_create_window);
        assert(NULL == error_create_window);
    }

    {
        xcb_void_cookie_t cookie_change_property_wm_protocols_delete_window = xcb_change_property_checked(s_connection, XCB_PROP_MODE_REPLACE, s_main_window, s_atom_wm_protocols, XCB_ATOM_ATOM, 8 * sizeof(uint32_t), sizeof(xcb_atom_t) / sizeof(uint32_t), &s_atom_wm_delete_window);

        xcb_generic_error_t *error_change_property_wm_protocols_delete_window = xcb_request_check(s_connection, cookie_change_property_wm_protocols_delete_window);
        assert(NULL == error_change_property_wm_protocols_delete_window);
    }

    {
        xcb_void_cookie_t cookie_change_property_wm_name = xcb_change_property_checked(s_connection, XCB_PROP_MODE_REPLACE, s_main_window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8 * sizeof(uint8_t), std::strlen(window_name), window_name);

        xcb_generic_error_t *error_change_property_net_wm_name = xcb_request_check(s_connection, cookie_change_property_wm_name);
        assert(NULL == error_change_property_net_wm_name);
    }

    internal_brx_wsi_set_window_icon(s_connection, s_main_window, s_icon_pixmap);

    internal_brx_wsi_set_window_size(s_connection, s_main_window, window_width, window_height);

    assert(0 == s_main_window_width);
    s_main_window_width = window_width;
    assert(0 != s_main_window_width);

    assert(0 == s_main_window_height);
    s_main_window_height = window_height;
    assert(0 != s_main_window_height);

    assert(NULL == s_pal_bridge_main_window.m_connection);
    s_pal_bridge_main_window.m_connection = s_connection;
    assert(NULL != s_pal_bridge_main_window.m_connection);

    assert(XCB_NONE == s_pal_bridge_main_window.m_window);
    s_pal_bridge_main_window.m_window = s_main_window;
    assert(XCB_NONE != s_pal_bridge_main_window.m_window);
}

extern "C" void brx_wsi_uninit_main_window()
{
    assert(NULL != s_pal_bridge_main_window.m_connection);
    s_pal_bridge_main_window.m_connection = NULL;

    assert(XCB_NONE != s_pal_bridge_main_window.m_window);
    s_pal_bridge_main_window.m_window = XCB_NONE;

    {
        assert(XCB_NONE != s_main_window);
        xcb_void_cookie_t cookie_destroy_window = xcb_destroy_window_checked(s_connection, s_main_window);
        s_main_window = XCB_NONE;

        xcb_generic_error_t *error_destroy_window = xcb_request_check(s_connection, cookie_destroy_window);
        assert(NULL == error_destroy_window);
    }

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
    return &s_pal_bridge_main_window;
}

extern "C" void brx_wsi_get_main_window_size(int *out_window_width, int *out_window_height)
{
    (*out_window_width) = s_main_window_width;
    (*out_window_height) = s_main_window_height;
}

extern "C" void brx_wsi_get_main_window_scale(float *out_window_width_scale, float *out_window_height_scale)
{
    (*out_window_width_scale) = 1.0F;
    (*out_window_height_scale) = 1.0F;
}

extern "C" void brx_wsi_show_main_window()
{
    {
        xcb_void_cookie_t cookie_map_window = xcb_map_window_checked(s_connection, s_main_window);

        xcb_generic_error_t *error_map_window = xcb_request_check(s_connection, cookie_map_window);
        assert(NULL == error_map_window);
    }
}

extern "C" void brx_wsi_set_main_window_size(int window_width, int window_height)
{
    if ((window_width != s_main_window_width) || (window_height != s_main_window_height))
    {
        s_main_window_width = window_width;
        s_main_window_height = window_height;

        internal_brx_wsi_set_window_size(s_connection, s_main_window, window_width, window_height);
    }
    else
    {
        assert(false);
    }
}

struct internal_brx_wsi_image_window
{
    xcb_window_t window;
    xcb_gcontext_t graphics_context;
    int32_t window_width;
    int32_t window_height;
};

extern "C" void *brx_wsi_create_image_window(char const *window_name)
{
    void *unwrapped_window_base = mcrt_malloc(sizeof(internal_brx_wsi_image_window), alignof(internal_brx_wsi_image_window));
    assert(NULL != unwrapped_window_base);

    internal_brx_wsi_image_window *unwrapped_window = new (unwrapped_window_base) internal_brx_wsi_image_window{XCB_NONE, XCB_NONE, 0, 0};
    assert(NULL != unwrapped_window);

    constexpr int32_t const k_window_width = 256;
    constexpr int32_t const k_window_height = 256;

    {
        assert(XCB_NONE == unwrapped_window->window);
        unwrapped_window->window = xcb_generate_id(s_connection);
        assert(XCB_NONE != unwrapped_window->window);

        // Both "border pixel" and "colormap" are required when the depth is NOT equal to the root window's.
        uint32_t value_mask = XCB_CW_BORDER_PIXEL | XCB_CW_BACKING_STORE | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;

        uint32_t value_list[4] = {0, XCB_BACKING_STORE_NOT_USEFUL, XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY, s_colormap};

        xcb_void_cookie_t cookie_create_window = xcb_create_window_checked(s_connection, k_depth, unwrapped_window->window, s_screen->root, 0, 0, k_window_width, k_window_height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, s_visual_id, value_mask, value_list);

        xcb_generic_error_t *error_create_window = xcb_request_check(s_connection, cookie_create_window);
        assert(NULL == error_create_window);
    }

    {
        xcb_void_cookie_t cookie_change_property_wm_protocols_delete_window = xcb_change_property_checked(s_connection, XCB_PROP_MODE_REPLACE, unwrapped_window->window, s_atom_wm_protocols, XCB_ATOM_ATOM, 8 * sizeof(uint32_t), sizeof(xcb_atom_t) / sizeof(uint32_t), &s_atom_wm_delete_window);

        xcb_generic_error_t *error_change_property_wm_protocols_delete_window = xcb_request_check(s_connection, cookie_change_property_wm_protocols_delete_window);
        assert(NULL == error_change_property_wm_protocols_delete_window);
    }

    {
        xcb_void_cookie_t cookie_change_property_wm_name = xcb_change_property_checked(s_connection, XCB_PROP_MODE_REPLACE, unwrapped_window->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8 * sizeof(uint8_t), std::strlen(window_name), window_name);

        xcb_generic_error_t *error_change_property_net_wm_name = xcb_request_check(s_connection, cookie_change_property_wm_name);
        assert(NULL == error_change_property_net_wm_name);
    }

    internal_brx_wsi_set_window_icon(s_connection, unwrapped_window->window, s_icon_pixmap);

    internal_brx_wsi_set_window_size(s_connection, unwrapped_window->window, k_window_width, k_window_height);

    {
        assert(XCB_NONE == unwrapped_window->graphics_context);
        unwrapped_window->graphics_context = xcb_generate_id(s_connection);
        assert(XCB_NONE != unwrapped_window->graphics_context);

        xcb_void_cookie_t cookie_create_graphics_context = xcb_create_gc_checked(s_connection, unwrapped_window->graphics_context, unwrapped_window->window, 0U, NULL);

        xcb_generic_error_t *error_create_graphics_context = xcb_request_check(s_connection, cookie_create_graphics_context);
        assert(NULL == error_create_graphics_context);
    }

    assert(0 == unwrapped_window->window_width);
    unwrapped_window->window_width = k_window_width;
    assert(0 != unwrapped_window->window_width);

    assert(0 == unwrapped_window->window_height);
    unwrapped_window->window_height = k_window_height;
    assert(0 != unwrapped_window->window_height);

    {
        xcb_void_cookie_t cookie_map_window = xcb_map_window_checked(s_connection, unwrapped_window->window);

        xcb_generic_error_t *error_map_window = xcb_request_check(s_connection, cookie_map_window);
        assert(NULL == error_map_window);
    }

    return unwrapped_window;
}

extern "C" void brx_wsi_destroy_image_window(void *wrapped_window)
{
    internal_brx_wsi_image_window *unwrapped_window = static_cast<internal_brx_wsi_image_window *>(wrapped_window);
    assert(NULL != unwrapped_window);

    {
        assert(XCB_NONE != unwrapped_window->graphics_context);
        xcb_void_cookie_t cookie_free_graphics_context = xcb_free_gc_checked(s_connection, unwrapped_window->graphics_context);
        unwrapped_window->graphics_context = XCB_NONE;

        xcb_generic_error_t *error_free_graphics_context = xcb_request_check(s_connection, cookie_free_graphics_context);
        assert(NULL == error_free_graphics_context);
    }

    {
        assert(XCB_NONE != unwrapped_window->window);
        xcb_void_cookie_t cookie_destroy_window = xcb_destroy_window_checked(s_connection, unwrapped_window->window);
        unwrapped_window->window = XCB_NONE;

        xcb_generic_error_t *error_destroy_window = xcb_request_check(s_connection, cookie_destroy_window);
        assert(NULL == error_destroy_window);
    }

    unwrapped_window->~internal_brx_wsi_image_window();
    mcrt_free(unwrapped_window);
}

extern "C" void brx_wsi_present_image_window(void *wrapped_window, void const *image_buffer, int image_width, int image_height)
{
    internal_brx_wsi_image_window *unwrapped_window = static_cast<internal_brx_wsi_image_window *>(wrapped_window);
    assert(NULL != unwrapped_window);

    assert(XCB_NONE != unwrapped_window->window);
    assert(XCB_NONE != unwrapped_window->graphics_context);

    if ((image_width != unwrapped_window->window_width) || (image_height != unwrapped_window->window_height))
    {
        internal_brx_wsi_set_window_size(s_connection, unwrapped_window->window, image_width, image_height);

        unwrapped_window->window_width = image_width;
        unwrapped_window->window_height = image_height;
    }

    // write "texture" into "window" directly
    assert((4U * s_maximum_request_length) > (sizeof(uint32_t) * image_width * image_height));
    xcb_put_image(s_connection, XCB_IMAGE_FORMAT_Z_PIXMAP, unwrapped_window->window, unwrapped_window->graphics_context, image_width, image_height, 0, 0, 0, k_depth, sizeof(uint32_t) * image_width * image_height, static_cast<uint8_t const *>(image_buffer));

    int result_flush = xcb_flush(s_connection);
    assert(result_flush > 0);
}

extern "C" void brx_wsi_run_main_loop(bool (*tick_callback)(void *tick_context), void *tick_context)
{
    bool running = true;

    while (running)
    {
        {
            xcb_generic_event_t *event;
            while ((event = xcb_poll_for_event(s_connection)) != NULL)
            {
                // The most significant bit(uint8_t(0X80)) in this code is set if the event was generated from a SendEvent request.
                // https://www.x.org/releases/current/doc/xproto/x11protocol.html#event_format
                switch (event->response_type & (~uint8_t(0X80)))
                {
                case XCB_KEY_PRESS:
                {
                    assert(XCB_KEY_PRESS == (event->response_type & (~uint8_t(0X80))));
                    xcb_key_press_event_t const *const key_press = reinterpret_cast<xcb_key_press_event_t *>(event);
                    if (key_press->event == s_main_window)
                    {
                        int const key = internal_evdev_key_to_imgui_key(key_press->detail);

                        bool const shift_key = (0U != (key_press->state & XCB_MOD_MASK_SHIFT));
                        bool const caps_key = (0U != (key_press->state & XCB_MOD_MASK_LOCK));
                        bool const ctrl_key = (0U != (key_press->state & XCB_MOD_MASK_CONTROL));
                        bool const alt_key = (0U != (key_press->state & XCB_MOD_MASK_1));

                        ImGui_ImplGLUT_KeyboardFunc(key, ctrl_key, shift_key, alt_key, caps_key);

                        s_main_window_key_press_handler(s_main_window_handler_context, key, shift_key, caps_key, ctrl_key, alt_key);
                    }
                }
                break;
                case XCB_KEY_RELEASE:
                {
                    assert(XCB_KEY_RELEASE == (event->response_type & (~uint8_t(0X80))));
                    xcb_key_release_event_t const *const key_release = reinterpret_cast<xcb_key_release_event_t *>(event);
                    if (key_release->event == s_main_window)
                    {
                        int const key = internal_evdev_key_to_imgui_key(key_release->detail);

                        bool const shift_key = (0U != (key_release->state & XCB_MOD_MASK_SHIFT));
                        bool const caps_key = (0U != (key_release->state & XCB_MOD_MASK_LOCK));
                        bool const ctrl_key = (0U != (key_release->state & XCB_MOD_MASK_CONTROL));
                        bool const alt_key = (0U != (key_release->state & XCB_MOD_MASK_1));

                        ImGui_ImplGLUT_KeyboardUpFunc(key, ctrl_key, shift_key, alt_key, caps_key);

                        s_main_window_key_release_handler(s_main_window_handler_context, key, shift_key, caps_key, ctrl_key, alt_key);
                    }
                }
                break;
                case XCB_BUTTON_PRESS:
                {
                    assert(XCB_BUTTON_PRESS == (event->response_type & (~uint8_t(0X80))));
                    xcb_button_press_event_t const *const button_press_event = reinterpret_cast<xcb_button_press_event_t *>(event);
                    if (button_press_event->event == s_main_window)
                    {
                        int const window_x = button_press_event->event_x;
                        int const window_y = button_press_event->event_y;

                        switch (button_press_event->detail)
                        {
                        case 1:
                        case 2:
                        case 3:
                        {
                            int const button = internal_xcb_button_to_imgui_button(button_press_event->detail);

                            ImGui_ImplGLUT_MouseFunc(button, true, window_x, window_y);

                            s_main_window_button_press_handler(s_main_window_handler_context, button, window_x, window_y);
                        }
                        break;
                        case 4:
                        {
                            ImGui_ImplGLUT_MouseWheelFunc(true, window_x, window_y);

                            s_main_window_scroll_up_handler(s_main_window_handler_context, window_x, window_y);
                        }
                        break;
                        case 5:
                        {
                            ImGui_ImplGLUT_MouseWheelFunc(false, window_x, window_y);

                            s_main_window_scroll_down_handler(s_main_window_handler_context, window_x, window_y);
                        }
                        break;
                        }
                    }
                }
                break;
                case XCB_BUTTON_RELEASE:
                {
                    assert(XCB_BUTTON_RELEASE == (event->response_type & (~uint8_t(0X80))));
                    xcb_button_release_event_t const *const button_release_event = reinterpret_cast<xcb_button_release_event_t *>(event);
                    if (button_release_event->event == s_main_window)
                    {
                        int const window_x = button_release_event->event_x;
                        int const window_y = button_release_event->event_y;

                        switch (button_release_event->detail)
                        {
                        case 1:
                        case 2:
                        case 3:
                        {
                            int const button = internal_xcb_button_to_imgui_button(button_release_event->detail);

                            ImGui_ImplGLUT_MouseFunc(button, false, window_x, window_y);

                            s_main_window_button_release_handler(s_main_window_handler_context, button, window_x, window_y);
                        }
                        break;
                        case 4:
                        case 5:
                        {
                            // Already Handled in Press Event
                        }
                        break;
                        }
                    }
                }
                break;
                case XCB_MOTION_NOTIFY:
                {
                    assert(XCB_MOTION_NOTIFY == (event->response_type & (~uint8_t(0X80))));
                    xcb_motion_notify_event_t const *const motion_notify = reinterpret_cast<xcb_motion_notify_event_t *>(event);
                    if (motion_notify->event == s_main_window)
                    {
                        int const window_x = motion_notify->event_x;
                        int const window_y = motion_notify->event_y;

                        bool const left_button = (0U != (motion_notify->state & XCB_BUTTON_MASK_1));
                        bool const middle_button = (0U != (motion_notify->state & XCB_BUTTON_MASK_2));
                        bool const right_button = (0U != (motion_notify->state & XCB_BUTTON_MASK_3));

                        ImGui_ImplGLUT_MotionFunc(window_x, window_y);

                        s_main_window_motion_handler(s_main_window_handler_context, window_x, window_y, left_button, middle_button, right_button);
                    }
                }
                break;
                case XCB_CONFIGURE_NOTIFY:
                {
                    assert(XCB_CONFIGURE_NOTIFY == (event->response_type & (~uint8_t(0X80))));
                    xcb_configure_notify_event_t const *const configure_notify = reinterpret_cast<xcb_configure_notify_event_t *>(event);
                    if (configure_notify->event == s_main_window)
                    {
                        uint16_t const new_width = configure_notify->width;
                        uint16_t const new_height = configure_notify->height;

                        // assert(new_width == s_main_window_width);
                        // assert(new_height == s_main_window_height);

                        ImGui_ImplGLUT_ReshapeFunc(new_width, new_height);

                        s_main_window_resize_handler(s_main_window_handler_context, new_width, new_height, 1.0F, 1.0F);
                    }
                }
                break;
                case XCB_CLIENT_MESSAGE:
                {
                    assert(XCB_CLIENT_MESSAGE == (event->response_type & (~uint8_t(0X80))));
                    xcb_client_message_event_t const *const client_message_event = reinterpret_cast<xcb_client_message_event_t *>(event);
                    assert((client_message_event->type == s_atom_wm_protocols) && (client_message_event->data.data32[0] == s_atom_wm_delete_window));
                    if (client_message_event->window == s_main_window)
                    {
                        running = false;
                    }
                }
                break;
                case XCB_NONE:
                {
                    assert(XCB_NONE == (event->response_type & (~uint8_t(0X80))));
                    assert(false);
                    // xcb_generic_error_t *error = reinterpret_cast<xcb_generic_error_t *>(event);
                    // std::printf("Error Code: %d Major Code: %d", static_cast<int>(error->error_code), static_cast<int>(error->major_code));
                }
                break;
                }

                std::free(event);
            }
        }

        if (running)
        {
            running = tick_callback(tick_context);
        }
    }
}

static inline void internal_brx_wsi_set_window_icon(xcb_connection_t *connection, xcb_window_t window, xcb_pixmap_t icon_pixmap)
{
    // xcb/xcb_icccm.h
    constexpr uint32_t const XCB_ICCCM_WM_HINT_ICON_PIXMAP = 1 << 2;

    struct wm_hints_t
    {
        int32_t flags;
        uint32_t input;
        int32_t initial_state;
        xcb_pixmap_t icon_pixmap;
        xcb_window_t icon_window;
        int32_t icon_x, icon_y;
        xcb_pixmap_t icon_mask;
        xcb_window_t window_group;
    };

    wm_hints_t ws_hints = {};
    ws_hints.flags = XCB_ICCCM_WM_HINT_ICON_PIXMAP;
    ws_hints.icon_pixmap = icon_pixmap;

    xcb_void_cookie_t cookie_change_property_wm_hints = xcb_change_property_checked(connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_HINTS, XCB_ATOM_WM_HINTS, 8 * sizeof(uint32_t), sizeof(wm_hints_t) / sizeof(uint32_t), &ws_hints);

    xcb_generic_error_t *error_change_property_wm_hints = xcb_request_check(connection, cookie_change_property_wm_hints);
    assert(NULL == error_change_property_wm_hints);
}

static inline void internal_brx_wsi_set_window_size(xcb_connection_t *connection, xcb_window_t window, int32_t width, int32_t height)
{
    {
        uint32_t value_mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;

        uint32_t value_list[2] = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        xcb_void_cookie_t cookie_configure_window = xcb_configure_window_checked(connection, window, value_mask, value_list);

        xcb_generic_error_t *error_configure_window = xcb_request_check(connection, cookie_configure_window);
        assert(NULL == error_configure_window);
    }

    {
        // xcb/xcb_icccm.h
        constexpr uint32_t const ICCCM_SIZE_HINT_P_MIN_SIZE = 1 << 4;
        constexpr uint32_t const ICCCM_SIZE_HINT_P_MAX_SIZE = 1 << 5;
        struct size_hints_t
        {
            uint32_t flags;
            int32_t x, y;
            int32_t width, height;
            int32_t min_width, min_height;
            int32_t max_width, max_height;
            int32_t width_inc, height_inc;
            int32_t min_aspect_num, min_aspect_den;
            int32_t max_aspect_num, max_aspect_den;
            int32_t base_width, base_height;
            uint32_t win_gravity;
        };

        size_hints_t size_hints = {};
        size_hints.flags = ICCCM_SIZE_HINT_P_MIN_SIZE | ICCCM_SIZE_HINT_P_MAX_SIZE;
        size_hints.min_width = width;
        size_hints.min_height = height;
        size_hints.max_width = width;
        size_hints.max_height = height;

        xcb_void_cookie_t cookie_change_property_size_hints = xcb_change_property_checked(connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NORMAL_HINTS, XCB_ATOM_WM_SIZE_HINTS, 8 * sizeof(uint32_t), sizeof(size_hints_t) / sizeof(uint32_t), &size_hints);

        xcb_generic_error_t *error_change_property_size_hints = xcb_request_check(connection, cookie_change_property_size_hints);
        assert(NULL == error_change_property_size_hints);
    }
}

static int internal_evdev_key_to_imgui_key(xcb_keycode_t evdev_key)
{
    // evdev keycode
    // https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config/-/blob/master/keycodes/evdev

    static constexpr ImGuiKey s_evdev_key_to_imgui_key_table[] = {
        ImGuiKey_None,           //   0
        ImGuiKey_None,           //   1
        ImGuiKey_None,           //   2
        ImGuiKey_None,           //   3
        ImGuiKey_None,           //   4
        ImGuiKey_None,           //   5
        ImGuiKey_None,           //   6
        ImGuiKey_None,           //   7
        ImGuiKey_None,           //   8
        ImGuiKey_Escape,         //   9 : <ESC>
        ImGuiKey_1,              //  10 : <AE01>
        ImGuiKey_2,              //  11 : <AE02>
        ImGuiKey_3,              //  12 : <AE03>
        ImGuiKey_4,              //  13 : <AE04>
        ImGuiKey_5,              //  14 : <AE05>
        ImGuiKey_6,              //  15 : <AE06>
        ImGuiKey_7,              //  16 : <AE07>
        ImGuiKey_8,              //  17 : <AE08>
        ImGuiKey_9,              //  18 : <AE09>
        ImGuiKey_0,              //  19 : <AE10>
        ImGuiKey_Minus,          //  20 : <AE11>
        ImGuiKey_Equal,          //  21 : <AE12>
        ImGuiKey_Backspace,      //  22 : <BKSP>
        ImGuiKey_Tab,            //  23 : <TAB>
        ImGuiKey_Q,              //  24 : <AD01>
        ImGuiKey_W,              //  25 : <AD02>
        ImGuiKey_E,              //  26 : <AD03>
        ImGuiKey_R,              //  27 : <AD04>
        ImGuiKey_T,              //  28 : <AD05>
        ImGuiKey_Y,              //  29 : <AD06>
        ImGuiKey_U,              //  30 : <AD07>
        ImGuiKey_I,              //  31 : <AD08>
        ImGuiKey_O,              //  32 : <AD09>
        ImGuiKey_P,              //  33 : <AD10>
        ImGuiKey_LeftBracket,    //  34 : <AD11>
        ImGuiKey_RightBracket,   //  35 : <AD12>
        ImGuiKey_Enter,          //  36 : <RTRN>
        ImGuiKey_LeftCtrl,       //  37 : <LCTL>
        ImGuiKey_A,              //  38 : <AC01>
        ImGuiKey_S,              //  39 : <AC02>
        ImGuiKey_D,              //  40 : <AC03>
        ImGuiKey_F,              //  41 : <AC04>
        ImGuiKey_G,              //  42 : <AC05>
        ImGuiKey_H,              //  43 : <AC06>
        ImGuiKey_J,              //  44 : <AC07>
        ImGuiKey_K,              //  45 : <AC08>
        ImGuiKey_L,              //  46 : <AC09>
        ImGuiKey_Semicolon,      //  47 : <AC10>
        ImGuiKey_Apostrophe,     //  48 : <AC11>
        ImGuiKey_GraveAccent,    //  49 : <TLDE>
        ImGuiKey_LeftShift,      //  50 : <LFSH>
        ImGuiKey_Backslash,      //  51 : <BKSL>
        ImGuiKey_Z,              //  52 : <AB01>
        ImGuiKey_X,              //  53 : <AB02>
        ImGuiKey_C,              //  54 : <AB03>
        ImGuiKey_V,              //  55 : <AB04>
        ImGuiKey_B,              //  56 : <AB05>
        ImGuiKey_N,              //  57 : <AB06>
        ImGuiKey_M,              //  58 : <AB07>
        ImGuiKey_Comma,          //  59 : <AB08>
        ImGuiKey_Period,         //  60 : <AB09>
        ImGuiKey_Slash,          //  61 : <AB10>
        ImGuiKey_RightShift,     //  62 : <RTSH>
        ImGuiKey_KeypadMultiply, //  63 : <KPMU>
        ImGuiKey_LeftAlt,        //  64 : <LALT>
        ImGuiKey_Space,          //  65 : <SPCE>
        ImGuiKey_CapsLock,       //  66 : <CAPS>
        ImGuiKey_F1,             //  67 : <FK01>
        ImGuiKey_F2,             //  68 : <FK02>
        ImGuiKey_F3,             //  69 : <FK03>
        ImGuiKey_F4,             //  70 : <FK04>
        ImGuiKey_F5,             //  71 : <FK05>
        ImGuiKey_F6,             //  72 : <FK06>
        ImGuiKey_F7,             //  73 : <FK07>
        ImGuiKey_F8,             //  74 : <FK08>
        ImGuiKey_F9,             //  75 : <FK09>
        ImGuiKey_F10,            //  76 : <FK10>
        ImGuiKey_NumLock,        //  77 : <NMLK>
        ImGuiKey_ScrollLock,     //  78 : <SCLK>
        ImGuiKey_Keypad7,        //  79 : <KP7>
        ImGuiKey_Keypad8,        //  80 : <KP8>
        ImGuiKey_Keypad9,        //  81 : <KP9>
        ImGuiKey_KeypadSubtract, //  82 : <KPSU>
        ImGuiKey_Keypad4,        //  83 : <KP4>
        ImGuiKey_Keypad5,        //  84 : <KP5>
        ImGuiKey_Keypad6,        //  85 : <KP6>
        ImGuiKey_KeypadAdd,      //  86 : <KPAD>
        ImGuiKey_Keypad1,        //  87 : <KP1>
        ImGuiKey_Keypad2,        //  88 : <KP2>
        ImGuiKey_Keypad3,        //  89 : <KP3>
        ImGuiKey_Keypad0,        //  90 : <KP0>
        ImGuiKey_KeypadDecimal,  //  91 : <KPDL>
        ImGuiKey_None,           //  92 : <LVL3> (AltGr level3, no direct ImGui key)
        ImGuiKey_None,           //  93 : <ZEHA> (Zenkaku/Hankaku)
        ImGuiKey_None,           //  94 : <LSGT> (ISO backslash/less-than)
        ImGuiKey_F11,            //  95 : <FK11>
        ImGuiKey_F12,            //  96 : <FK12>
        ImGuiKey_None,           //  97 : <AB11> (ISO extra key)
        ImGuiKey_None,           //  98 : <KATA>
        ImGuiKey_None,           //  99 : <HIRA>
        ImGuiKey_None,           // 100 : <HENK>
        ImGuiKey_None,           // 101 : <HKTG>
        ImGuiKey_None,           // 102 : <MUHE>
        ImGuiKey_None,           // 103 : <JPCM>
        ImGuiKey_KeypadEnter,    // 104 : <KPEN>
        ImGuiKey_RightCtrl,      // 105 : <RCTL>
        ImGuiKey_KeypadDivide,   // 106 : <KPDV>
        ImGuiKey_PrintScreen,    // 107 : <PRSC>
        ImGuiKey_RightAlt,       // 108 : <RALT>
        ImGuiKey_None,           // 109 : <LNFD>
        ImGuiKey_Home,           // 110 : <HOME>
        ImGuiKey_UpArrow,        // 111 : <UP>
        ImGuiKey_PageUp,         // 112 : <PGUP>
        ImGuiKey_LeftArrow,      // 113 : <LEFT>
        ImGuiKey_RightArrow,     // 114 : <RGHT>
        ImGuiKey_End,            // 115 : <END>
        ImGuiKey_DownArrow,      // 116 : <DOWN>
        ImGuiKey_PageDown,       // 117 : <PGDN>
        ImGuiKey_Insert,         // 118 : <INS>
        ImGuiKey_Delete,         // 119 : <DELE>
        ImGuiKey_None,           // 120 : <I120>
        ImGuiKey_None,           // 121 : <MUTE>
        ImGuiKey_None,           // 122
        ImGuiKey_None,           // 123
        ImGuiKey_None,           // 124 : <POWR>
        ImGuiKey_KeypadEqual,    // 125 : <KPEQ>
        ImGuiKey_None,           // 126 : <I126>
        ImGuiKey_Pause,          // 127 : <PAUS>
        ImGuiKey_None,           // 128 : <I128>
        ImGuiKey_None,           // 129 : <I129>
        ImGuiKey_None,           // 130 : <HNGL>
        ImGuiKey_None,           // 131 : <HJCV>
        ImGuiKey_None,           // 132 : <AE13>
        ImGuiKey_LeftSuper,      // 133 : <LWIN>
        ImGuiKey_RightSuper,     // 134 : <RWIN>
        ImGuiKey_None,           // 135 : <COMP> (Compose)
        ImGuiKey_None,           // 136 : <STOP>
        ImGuiKey_None,           // 137 : <AGAI>
        ImGuiKey_None,           // 138 : <PROP>
        ImGuiKey_None,           // 139 : <UNDO>
        ImGuiKey_None,           // 140 : <FRNT>
        ImGuiKey_None,           // 141 : <COPY>
        ImGuiKey_None,           // 142 : <OPEN>
        ImGuiKey_None,           // 143 : <PAST>
        ImGuiKey_None,           // 144 : <FIND>
        ImGuiKey_None,           // 145 : <CUT>
        ImGuiKey_None,           // 146 : <HELP>
        ImGuiKey_Menu,           // 147 : <I147> (KEY_MENU)
        ImGuiKey_None,           // 148 : <I148>
        ImGuiKey_None,           // 149
        ImGuiKey_None,           // 150
        ImGuiKey_None,           // 151
        ImGuiKey_None,           // 152
        ImGuiKey_None,           // 153
        ImGuiKey_None,           // 154
        ImGuiKey_None,           // 155
        ImGuiKey_None,           // 156
        ImGuiKey_None,           // 157
        ImGuiKey_None,           // 158
        ImGuiKey_None,           // 159
        ImGuiKey_None,           // 160
        ImGuiKey_None,           // 161
        ImGuiKey_None,           // 162
        ImGuiKey_None,           // 163
        ImGuiKey_None,           // 164
        ImGuiKey_None,           // 165
        ImGuiKey_None,           // 166
        ImGuiKey_None,           // 167
        ImGuiKey_None,           // 168
        ImGuiKey_None,           // 169
        ImGuiKey_None,           // 170
        ImGuiKey_None,           // 171
        ImGuiKey_None,           // 172
        ImGuiKey_None,           // 173
        ImGuiKey_None,           // 174
        ImGuiKey_None,           // 175
        ImGuiKey_None,           // 176
        ImGuiKey_None,           // 177
        ImGuiKey_None,           // 178
        ImGuiKey_None,           // 179
        ImGuiKey_None,           // 180
        ImGuiKey_None,           // 181
        ImGuiKey_None,           // 182
        ImGuiKey_F13,            // 183 : <FK13>
        ImGuiKey_F14,            // 184 : <FK14>
        ImGuiKey_F15,            // 185 : <FK15>
        ImGuiKey_F16,            // 186 : <FK16>
        ImGuiKey_F17,            // 187 : <FK17>
        ImGuiKey_F18,            // 188 : <FK18>
        ImGuiKey_F19,            // 189 : <FK19>
        ImGuiKey_F20,            // 190 : <FK20>
        ImGuiKey_F21,            // 191 : <FK21>
        ImGuiKey_F22,            // 192 : <FK22>
        ImGuiKey_F23,            // 193 : <FK23>
        ImGuiKey_F24,            // 194 : <FK24>
        ImGuiKey_None,           // 195
        ImGuiKey_None,           // 196
        ImGuiKey_None,           // 197
        ImGuiKey_None,           // 198
        ImGuiKey_None,           // 199
        ImGuiKey_None,           // 200
        ImGuiKey_None,           // 201
        ImGuiKey_None,           // 202
        ImGuiKey_None,           // 203
        ImGuiKey_None,           // 204
        ImGuiKey_None,           // 205
        ImGuiKey_None,           // 206
        ImGuiKey_None,           // 207
        ImGuiKey_None,           // 208
        ImGuiKey_None,           // 209
        ImGuiKey_None,           // 210
        ImGuiKey_None,           // 211
        ImGuiKey_None,           // 212
        ImGuiKey_None,           // 213
        ImGuiKey_None,           // 214
        ImGuiKey_None,           // 215
        ImGuiKey_None,           // 216
        ImGuiKey_None,           // 217
        ImGuiKey_None,           // 218
        ImGuiKey_None,           // 219
        ImGuiKey_None,           // 220
        ImGuiKey_None,           // 221
        ImGuiKey_None,           // 222
        ImGuiKey_None,           // 223
        ImGuiKey_None,           // 224
        ImGuiKey_None,           // 225
        ImGuiKey_None,           // 226
        ImGuiKey_None,           // 227
        ImGuiKey_None,           // 228
        ImGuiKey_None,           // 229
        ImGuiKey_None,           // 230
        ImGuiKey_None,           // 231
        ImGuiKey_None,           // 232
        ImGuiKey_None,           // 233
        ImGuiKey_None,           // 234
        ImGuiKey_None,           // 235
        ImGuiKey_None,           // 236
        ImGuiKey_None,           // 237
        ImGuiKey_None,           // 238
        ImGuiKey_None,           // 239
        ImGuiKey_None,           // 240
        ImGuiKey_None,           // 241
        ImGuiKey_None,           // 242
        ImGuiKey_None,           // 243
        ImGuiKey_None,           // 244
        ImGuiKey_None,           // 245
        ImGuiKey_None,           // 246
        ImGuiKey_None,           // 247
        ImGuiKey_None,           // 248
        ImGuiKey_None,           // 249
        ImGuiKey_None,           // 250
        ImGuiKey_None,           // 251
        ImGuiKey_None,           // 252
        ImGuiKey_None,           // 253
        ImGuiKey_None,           // 254
        ImGuiKey_None            // 255
    };

    static_assert(((sizeof(s_evdev_key_to_imgui_key_table) / sizeof(s_evdev_key_to_imgui_key_table[0])) == 256), "");

    return s_evdev_key_to_imgui_key_table[static_cast<uint8_t>(evdev_key)];
}

static int internal_xcb_button_to_imgui_button(xcb_button_t xcb_button)
{
    static constexpr ImGuiMouseButton s_xcb_button_to_imgui_button_table[] = {
        ImGuiMouseButton_COUNT,  //   0
        ImGuiMouseButton_Left,   //   1
        ImGuiMouseButton_Middle, //   2
        ImGuiMouseButton_Right   //   3
    };

    static_assert(((sizeof(s_xcb_button_to_imgui_button_table) / sizeof(s_xcb_button_to_imgui_button_table[0])) == 4), "");

    return s_xcb_button_to_imgui_button_table[static_cast<uint8_t>(xcb_button)];
}

static mcrt_string s_file_name = secure_getenv("HOME");

extern "C" bool brx_wsi_get_open_file_name(int filter_count, char const *const *filter_names, char const *const *filter_specs, int *inout_filter_index, void *out_file_name_std_string, void (*std_string_assign_callback)(void *out_file_name_std_string, char const *s))
{
    mcrt_string file_name;
    bool open_file = false;
    {
        mcrt_vector<char *> argv(static_cast<size_t>(3U + filter_count + 1U));

        mcrt_string argv_0;
        {
            char dir_name[4096] = {};
            {
                ssize_t res_read_link = readlink("/proc/self/exe", dir_name, (sizeof(dir_name) / sizeof(dir_name[0])));
                assert(-1 != res_read_link);

                assert(res_read_link < (sizeof(dir_name) / sizeof(dir_name[0])));

                for (ssize_t index_plus_one = res_read_link; index_plus_one > 0; --index_plus_one)
                {
                    ssize_t index = index_plus_one - 1;

                    if ('/' != dir_name[index])
                    {
                        dir_name[index] = '\0';
                    }
                    else
                    {
                        break;
                    }
                }
            }

            argv_0 = dir_name;
            argv_0 += "BRX-OpenFileDialog";
        }
        argv[0] = argv_0.data();

        argv[1] = s_file_name.data();

        mcrt_string argv_2;
        {
            char in_name_filer_index_text[] = {"-2147483647"};
            std::snprintf(in_name_filer_index_text, sizeof(in_name_filer_index_text) / sizeof(in_name_filer_index_text[0]), "%" PRId32, static_cast<int32_t>(*inout_filter_index));

            argv_2 = in_name_filer_index_text;
        }
        argv[2] = argv_2.data();

        mcrt_vector<mcrt_string> file_types(static_cast<size_t>(filter_count));
        for (uint32_t filter_index = 0U; filter_index < filter_count; ++filter_index)
        {
            mcrt_string modified_filter_spec = filter_specs[filter_index];
            std::replace(modified_filter_spec.begin(), modified_filter_spec.end(), ';', ' ');

            file_types[filter_index] = filter_names[filter_index];
            file_types[filter_index] += " (";
            file_types[filter_index] += modified_filter_spec;
            file_types[filter_index] += ")";

            argv[3U + filter_index] = file_types[filter_index].data();
        }

        argv[3U + filter_count] = NULL;

        int socket_vector[2];
        {
            int res_socketpair = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_vector);
            assert(0 == res_socketpair);
        }

        {
            int flags = fcntl(socket_vector[0], F_GETFD);
            assert(-1 != flags);

            int res_fcntl_setfd = fcntl(socket_vector[0], F_SETFD, (flags | FD_CLOEXEC));
            assert(0 == res_fcntl_setfd);
        }

        {
            int flags = fcntl(socket_vector[1], F_GETFD);
            assert(-1 != flags);

            int res_fcntl_setfd = fcntl(socket_vector[1], F_SETFD, (flags | FD_CLOEXEC));
            assert(0 == res_fcntl_setfd);
        }

        pid_t child_pid = fork();
        if (0 == child_pid)
        {
            {
                int res_close = close(socket_vector[0]);
                assert(0 == res_close);
            }

            {
                int res_close = close(STDOUT_FILENO);
                assert(0 == res_close);
            }

            {
                int res_dup2 = dup2(socket_vector[1], STDOUT_FILENO);
                assert(STDOUT_FILENO == res_dup2);
            }

            {
                int res_close = close(socket_vector[1]);
                assert(0 == res_close);
            }

            {
                int res_execvp = execvp(argv[0], argv.data());
                assert(-1 == res_execvp);
            }

            // fail execvp
            // no return
            _exit(-1);
        }

        if (child_pid > 0)
        {
            mcrt_vector<char> child_stdout;
            {
                {
                    int res_close = close(socket_vector[1]);
                    assert(0 == res_close);
                }

                assert(child_stdout.empty());
                while (true)
                {
                    char buffer_read[4096];
                    ssize_t size_read = read(socket_vector[0], buffer_read, sizeof(buffer_read));
                    if (size_read > 0)
                    {
                        size_t offset = child_stdout.size();
                        child_stdout.resize(offset + size_read);
                        std::memcpy(child_stdout.data() + offset, buffer_read, size_read);
                    }
                    else if (0 == size_read)
                    {
                        // EOF
                        break;
                    }
                    else if ((-1 == size_read) && (EINTR == errno))
                    {
                        // Do Nothing
                    }
                    else
                    {
                        child_stdout.clear();
                        assert(child_stdout.empty());
                        break;
                    }
                }

                {
                    int res_close = close(socket_vector[0]);
                    assert(0 == res_close);
                }

                while (true)
                {
                    int stat_loc;
                    pid_t res_waitpid = waitpid(child_pid, &stat_loc, 0);
                    if (res_waitpid > 0)
                    {
                        if ((WIFEXITED(stat_loc)) && (0 == (WEXITSTATUS(stat_loc))))
                        {
                            // Do Nothing
                        }
                        else
                        {
                            child_stdout.clear();
                            assert(child_stdout.empty());
                        }

                        break;
                    }
                    else if ((-1 == res_waitpid) && (EINTR == errno))
                    {
                        // Do Nothing
                    }
                    else
                    {
                        child_stdout.clear();
                        assert(child_stdout.empty());
                        break;
                    }
                }
            }

            if (!child_stdout.empty())
            {
                mcrt_string result_name_filer_index_text;
                mcrt_string result_file_name;
                {
                    char *result_name_filer_index_text_end = static_cast<char *>(std::memchr(child_stdout.data(), '\0', child_stdout.size()));
                    if (NULL != result_name_filer_index_text_end)
                    {
                        result_name_filer_index_text.assign(child_stdout.data(), result_name_filer_index_text_end - child_stdout.data());

                        char *result_file_name_begin = result_name_filer_index_text_end + 1;
                        if (result_file_name_begin < (child_stdout.data() + child_stdout.size()))
                        {
                            char *result_file_name_end = static_cast<char *>(std::memchr(result_file_name_begin, '\0', (child_stdout.data() + child_stdout.size()) - result_file_name_begin));

                            if (NULL != result_file_name_end)
                            {
                                assert((child_stdout.data() + child_stdout.size()) == (result_file_name_end + 1));
                                result_file_name.assign(result_file_name_begin, result_file_name_end - result_file_name_begin);
                            }
                        }
                    }
                }

                (*inout_filter_index) = std::strtoll(result_name_filer_index_text.c_str(), NULL, 10);

                assert(file_name.empty());
                file_name = std::move(result_file_name);

                s_file_name = file_name;

                open_file = true;
            }
        }
        else
        {
            {
                int res_close = close(socket_vector[0]);
                assert(0 == res_close);
            }

            {
                int res_close = close(socket_vector[1]);
                assert(0 == res_close);
            }

            assert(file_name.empty());
        }
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
    assert(false);
}
