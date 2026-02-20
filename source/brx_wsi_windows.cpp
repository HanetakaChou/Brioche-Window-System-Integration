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
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <sdkddkver.h>
#include <Windows.h>
#include <windowsx.h>
#include <shobjidl_core.h>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>
#include "../../Brioche-ImGui/imgui.h"
#include "../../McRT-Malloc/include/mcrt_vector.h"
#include "../../McRT-Malloc/include/mcrt_string.h"
#include "../../libiconv/include/iconv.h"
#include "../build-windows/resource.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

static ATOM s_main_window_class = 0U;

static LRESULT CALLBACK s_main_wnd_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

static ATOM s_image_window_class = 0U;

static LRESULT CALLBACK s_image_wnd_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

extern "C" void brx_wsi_init_connection()
{
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);

    {
        assert(0U == s_main_window_class);
        WNDCLASSEXW const window_class_create_info = {
            sizeof(WNDCLASSEX),
            CS_OWNDC,
            s_main_wnd_proc,
            0,
            0,
            hInstance,
            LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON_BRX_PUPPET)),
            LoadCursorW(hInstance, IDC_ARROW),
            (HBRUSH)GetStockObject(NULL_BRUSH),
            NULL,
            L"Brioche-Window-System-Integration::Main-Window",
            LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON_BRX_PUPPET)),
        };
        s_main_window_class = RegisterClassExW(&window_class_create_info);
        assert(0 != s_main_window_class);
    }

    {
        assert(0U == s_image_window_class);
        WNDCLASSEXW const window_class_create_info = {
            sizeof(WNDCLASSEX),
            CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_NOCLOSE,
            s_image_wnd_proc,
            0,
            0,
            hInstance,
            LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON_BRX_PUPPET)),
            LoadCursorW(hInstance, IDC_ARROW),
            (HBRUSH)GetStockObject(NULL_BRUSH),
            NULL,
            L"Brioche-Window-System-Integration::Image-Window",
            LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON_BRX_PUPPET)),
        };
        s_image_window_class = RegisterClassExW(&window_class_create_info);
        assert(0 != s_image_window_class);
    }
}

extern "C" void brx_wsi_uninit_connection()
{
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);

    {
        assert(0U != s_image_window_class);
        BOOL res_unregister_class = UnregisterClassW(MAKEINTATOM(s_image_window_class), hInstance);
        assert(FALSE != res_unregister_class);
        s_image_window_class = 0U;
    }

    {
        assert(0U != s_main_window_class);
        BOOL res_unregister_class = UnregisterClassW(MAKEINTATOM(s_main_window_class), hInstance);
        assert(FALSE != res_unregister_class);
        s_main_window_class = 0U;
    }
}

extern "C" void *brx_wsi_get_connection()
{
    return NULL;
}

static constexpr DWORD const s_main_window_style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
static constexpr DWORD const s_main_window_ex_style = WS_EX_APPWINDOW;

static HWND s_main_window = NULL;
static int32_t s_main_window_width = 0;
static int32_t s_main_window_height = 0;

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
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);

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
        assert(0U == s_main_window);

        mcrt_wstring window_name_utf16;
        {
            assert(window_name_utf16.empty());

            mcrt_string src_utf8 = window_name;
            mcrt_wstring &dst_utf16 = window_name_utf16;

            assert(dst_utf16.empty());

            if (!src_utf8.empty())
            {
                // Allocate the same number of UTF-16 code units as UTF-8 code units. Encoding
                // as UTF-16 should always require the same amount or less code units than the
                // UTF-8 encoding.  Allocate one extra byte for the null terminator though,
                // so that someone calling DstUTF16.data() gets a null terminated string.
                // We resize down later so we don't have to worry that this over allocates.
                dst_utf16.resize(src_utf8.size() + 1U);

                size_t in_bytes_left = sizeof(src_utf8[0]) * src_utf8.size();
                size_t out_bytes_left = sizeof(dst_utf16[0]) * dst_utf16.size();
                char *in_buf = src_utf8.data();
                char *out_buf = reinterpret_cast<char *>(dst_utf16.data());

                iconv_t conversion_descriptor = iconv_open("UTF-16LE", "UTF-8");
                assert(((iconv_t)(-1)) != conversion_descriptor);

                size_t conversion_result = iconv(conversion_descriptor, &in_buf, &in_bytes_left, &out_buf, &out_bytes_left);
                assert(((size_t)(-1)) != conversion_result);

                int result = iconv_close(conversion_descriptor);
                assert(-1 != result);

                dst_utf16.resize(reinterpret_cast<decltype(&dst_utf16[0])>(out_buf) - dst_utf16.data());
            }
        }

        HWND const desktop_window = GetDesktopWindow();

        RECT rect;
        {
            HMONITOR const monitor = MonitorFromWindow(desktop_window, MONITOR_DEFAULTTONEAREST);

            MONITORINFOEXW monitor_info;
            monitor_info.cbSize = sizeof(MONITORINFOEXW);
            BOOL res_get_monitor_info = GetMonitorInfoW(monitor, &monitor_info);
            assert(FALSE != res_get_monitor_info);

            rect = RECT{(monitor_info.rcWork.left + monitor_info.rcWork.right) / 2 - window_width / 2,
                        (monitor_info.rcWork.bottom + monitor_info.rcWork.top) / 2 - window_height / 2,
                        (monitor_info.rcWork.left + monitor_info.rcWork.right) / 2 + window_width / 2,
                        (monitor_info.rcWork.bottom + monitor_info.rcWork.top) / 2 + window_height / 2};

            BOOL const res_adjust_window_rest = AdjustWindowRectEx(&rect, s_main_window_style, FALSE, s_main_window_ex_style);
            assert(FALSE != res_adjust_window_rest);
        }

        s_main_window = CreateWindowExW(s_main_window_ex_style, MAKEINTATOM(s_main_window_class), window_name_utf16.c_str(), s_main_window_style, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, desktop_window, NULL, hInstance, NULL);
        assert(NULL != s_main_window);
    }

    assert(0 == s_main_window_width);
    s_main_window_width = window_width;
    assert(0 != s_main_window_width);

    assert(0 == s_main_window_height);
    s_main_window_height = window_height;
    assert(0 != s_main_window_height);
}

extern "C" void brx_wsi_uninit_main_window()
{
    assert(NULL != s_main_window);
    BOOL res_destroy_window = DestroyWindow(s_main_window);
    assert(FALSE != res_destroy_window);
    s_main_window = NULL;

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
    return s_main_window;
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
    ShowWindow(s_main_window, SW_SHOWDEFAULT);

    {
        BOOL result_update_window = UpdateWindow(s_main_window);
        assert(FALSE != result_update_window);
    }
}

extern "C" void brx_wsi_set_main_window_size(int window_width, int window_height)
{
    if ((window_width != s_main_window_width) || (window_height != s_main_window_height))
    {
        s_main_window_width = window_width;
        s_main_window_height = window_height;

        {
            RECT rect;
            {
                rect = RECT{0, 0, window_width, window_height};

                BOOL const res_adjust_window_rest = AdjustWindowRectEx(&rect, s_main_window_style, FALSE, s_main_window_ex_style);
                assert(FALSE != res_adjust_window_rest);
            }

            BOOL res_set_window_pos = SetWindowPos(s_main_window, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            assert(FALSE != res_set_window_pos);
        }
    }
    else
    {
        assert(false);
    }
}

extern "C" IMGUI_IMPL_API ImGuiKey ImGui_ImplWin32_KeyEventToImGuiKey(WPARAM wParam, LPARAM lParam);

extern "C" IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK s_main_wnd_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    assert((NULL == s_main_window) || (hWnd == s_main_window));

    LRESULT res_imgui = ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);

    switch (Msg)
    {
    case WM_MOUSEMOVE:
    {
        int const window_x = GET_X_LPARAM(lParam);
        int const window_y = GET_Y_LPARAM(lParam);

        bool const leff_button = (0U != (wParam & MK_LBUTTON));
        bool const middle_button = (0U != (wParam & MK_MBUTTON));
        bool const right_button = (0U != (wParam & MK_RBUTTON));

        s_main_window_motion_handler(s_main_window_handler_context, window_x, window_y, leff_button, middle_button, right_button);
    }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_KEYDOWN:
    {
        int const key = ImGui_ImplWin32_KeyEventToImGuiKey(wParam, lParam);

        bool shift_key = (0 != (GetKeyState(VK_SHIFT) & 0X8000));
        bool ctrl_key = (0 != (GetKeyState(VK_CONTROL) & 0X8000));
        bool alt_key = (0 != (GetKeyState(VK_MENU) & 0X8000));
        bool caps_key = (0 != (GetKeyState(VK_CAPITAL) & 1));

        s_main_window_key_press_handler(s_main_window_handler_context, key, shift_key, caps_key, ctrl_key, alt_key);
    }
        return 0;
    case WM_KEYUP:
    {
        int const key = ImGui_ImplWin32_KeyEventToImGuiKey(wParam, lParam);

        bool shift_key = (0 != (GetKeyState(VK_SHIFT) & 0X8000));
        bool ctrl_key = (0 != (GetKeyState(VK_CONTROL) & 0X8000));
        bool alt_key = (0 != (GetKeyState(VK_MENU) & 0X8000));
        bool caps_key = (0 != (GetKeyState(VK_CAPITAL) & 1));

        s_main_window_key_release_handler(s_main_window_handler_context, key, shift_key, caps_key, ctrl_key, alt_key);
    }
        return 0;
    case WM_SIZE:
    {
        WORD const new_width = LOWORD(lParam);
        WORD const new_height = HIWORD(lParam);

        assert((0U == new_width) || (new_width == s_main_window_width));
        assert((0U == new_height) || (new_height == s_main_window_height));

        s_main_window_resize_handler(s_main_window_handler_context, new_width, new_height, 1.0F, 1.0F);
    }
        return 0;
    case WM_CLOSE:
    {
        PostQuitMessage(0);
    }
        return 0;
    default:
        if (0 != res_imgui)
        {
            return res_imgui;
        }
        else
        {
            return DefWindowProcW(hWnd, Msg, wParam, lParam);
        }
    }
}

struct internal_brx_wsi_image_window
{
    HWND window;
    HDC device_context;
    int32_t window_width;
    int32_t window_height;
};

static constexpr DWORD const s_image_window_style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
static constexpr DWORD const s_image_window_ex_style = WS_EX_APPWINDOW;

extern "C" void *brx_wsi_create_image_window(char const *window_name)
{
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);

    void *unwrapped_window_base = mcrt_malloc(sizeof(internal_brx_wsi_image_window), alignof(internal_brx_wsi_image_window));
    assert(NULL != unwrapped_window_base);

    internal_brx_wsi_image_window *unwrapped_window = new (unwrapped_window_base) internal_brx_wsi_image_window{NULL, NULL, 0, 0};
    assert(NULL != unwrapped_window);

    constexpr int32_t const k_window_width = 256;
    constexpr int32_t const k_window_height = 256;

    {
        mcrt_wstring window_name_utf16;
        {
            assert(window_name_utf16.empty());

            mcrt_string src_utf8 = window_name;
            mcrt_wstring &dst_utf16 = window_name_utf16;

            assert(dst_utf16.empty());

            if (!src_utf8.empty())
            {
                // Allocate the same number of UTF-16 code units as UTF-8 code units. Encoding
                // as UTF-16 should always require the same amount or less code units than the
                // UTF-8 encoding.  Allocate one extra byte for the null terminator though,
                // so that someone calling DstUTF16.data() gets a null terminated string.
                // We resize down later so we don't have to worry that this over allocates.
                dst_utf16.resize(src_utf8.size() + 1U);

                size_t in_bytes_left = sizeof(src_utf8[0]) * src_utf8.size();
                size_t out_bytes_left = sizeof(dst_utf16[0]) * dst_utf16.size();
                char *in_buf = src_utf8.data();
                char *out_buf = reinterpret_cast<char *>(dst_utf16.data());

                iconv_t conversion_descriptor = iconv_open("UTF-16LE", "UTF-8");
                assert(((iconv_t)(-1)) != conversion_descriptor);

                size_t conversion_result = iconv(conversion_descriptor, &in_buf, &in_bytes_left, &out_buf, &out_bytes_left);
                assert(((size_t)(-1)) != conversion_result);

                int result = iconv_close(conversion_descriptor);
                assert(-1 != result);

                dst_utf16.resize(reinterpret_cast<decltype(&dst_utf16[0])>(out_buf) - dst_utf16.data());
            }
        }

        HWND const desktop_window = GetDesktopWindow();

        RECT rect;
        {
            HMONITOR const monitor = MonitorFromWindow(desktop_window, MONITOR_DEFAULTTONEAREST);

            MONITORINFOEXW monitor_info;
            monitor_info.cbSize = sizeof(MONITORINFOEXW);
            BOOL res_get_monitor_info = GetMonitorInfoW(monitor, &monitor_info);
            assert(FALSE != res_get_monitor_info);

            rect = RECT{(monitor_info.rcWork.left + monitor_info.rcWork.right) / 2 - k_window_width / 2,
                        (monitor_info.rcWork.bottom + monitor_info.rcWork.top) / 2 - k_window_height / 2,
                        (monitor_info.rcWork.left + monitor_info.rcWork.right) / 2 + k_window_width / 2,
                        (monitor_info.rcWork.bottom + monitor_info.rcWork.top) / 2 + k_window_height / 2};

            BOOL const res_adjust_window_rest = AdjustWindowRectEx(&rect, s_image_window_style, FALSE, s_image_window_ex_style);
            assert(FALSE != res_adjust_window_rest);
        }

        assert(NULL == unwrapped_window->window);
        unwrapped_window->window = CreateWindowExW(s_image_window_ex_style, MAKEINTATOM(s_image_window_class), window_name_utf16.c_str(), s_image_window_style, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, desktop_window, NULL, hInstance, NULL);
        assert(NULL != unwrapped_window->window);
    }

    assert(NULL == unwrapped_window->device_context);
    unwrapped_window->device_context = GetDC(unwrapped_window->window);
    assert(NULL != unwrapped_window->device_context);

    assert(0 == unwrapped_window->window_width);
    unwrapped_window->window_width = k_window_width;
    assert(0 != unwrapped_window->window_width);

    assert(0 == unwrapped_window->window_height);
    unwrapped_window->window_height = k_window_height;
    assert(0 != unwrapped_window->window_height);

    ShowWindow(unwrapped_window->window, SW_SHOWDEFAULT);

    {
        BOOL result_update_window = UpdateWindow(unwrapped_window->window);
        assert(FALSE != result_update_window);
    }

    return static_cast<void *>(unwrapped_window);
}

extern "C" void brx_wsi_destroy_image_window(void *wrapped_window)
{
    internal_brx_wsi_image_window *unwrapped_window = static_cast<internal_brx_wsi_image_window *>(wrapped_window);
    assert(NULL != unwrapped_window);

    assert(NULL != unwrapped_window->device_context);
    BOOL result_release_dc = ReleaseDC(unwrapped_window->window, unwrapped_window->device_context);
    assert(FALSE != result_release_dc);
    unwrapped_window->device_context = NULL;

    assert(NULL != unwrapped_window->window);
    BOOL res_destroy_window = DestroyWindow(unwrapped_window->window);
    assert(FALSE != res_destroy_window);
    unwrapped_window->window = NULL;

    unwrapped_window->~internal_brx_wsi_image_window();
    mcrt_free(unwrapped_window);
}

extern "C" void brx_wsi_present_image_window(void *wrapped_window, void const *image_buffer, int image_width, int image_height)
{
    internal_brx_wsi_image_window *unwrapped_window = static_cast<internal_brx_wsi_image_window *>(wrapped_window);
    assert(NULL != unwrapped_window);

    assert(NULL != unwrapped_window->window);
    assert(NULL != unwrapped_window->device_context);

    if ((image_width != unwrapped_window->window_width) || (image_height != unwrapped_window->window_height))
    {
        {
            RECT rect;
            {
                rect = RECT{0, 0, image_width, image_height};

                BOOL const res_adjust_window_rest = AdjustWindowRectEx(&rect, s_image_window_style, FALSE, s_image_window_ex_style);
                assert(FALSE != res_adjust_window_rest);
            }

            BOOL res_set_window_pos = SetWindowPos(unwrapped_window->window, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            assert(FALSE != res_set_window_pos);
        }

        unwrapped_window->window_width = image_width;
        unwrapped_window->window_height = image_height;
    }

    {
        // write "texture" into "window" directly
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = image_width;
        bmi.bmiHeader.biHeight = -image_height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        int result_set_dib_bits = SetDIBitsToDevice(unwrapped_window->device_context, 0, 0, image_width, image_height, 0, 0, 0, image_height, image_buffer, &bmi, DIB_RGB_COLORS);
        assert(result_set_dib_bits > 0);
    }
}

static LRESULT CALLBACK s_image_wnd_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result;
    switch (Msg)
    {
    case WM_ERASEBKGND:
    {
        assert(WM_ERASEBKGND == Msg);
        result = 1;
    }
    break;
    case WM_CLOSE:
    {
        assert(WM_CLOSE == Msg);
        result = 0;
    }
    break;
    default:
    {
        assert((WM_ERASEBKGND != Msg) && (WM_CLOSE != Msg));
        result = DefWindowProcW(hWnd, Msg, wParam, lParam);
    }
    }
    return result;
}

extern "C" void brx_wsi_run_main_loop(bool (*tick_callback)(void *tick_context), void *tick_context)
{
    bool running = true;

    while (running)
    {
        {
            MSG msg;
            while (PeekMessageW(&msg, NULL, 0U, 0U, PM_REMOVE))
            {
                if (WM_QUIT == msg.message)
                {
                    running = false;
                }

                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        if (running)
        {
            running = tick_callback(tick_context);
        }
    }
}

static inline HRESULT NoRegCoCreate(const __wchar_t *dllName, REFCLSID rclsid, REFIID riid, void **ppv)
{
    HMODULE dynamic_library = GetModuleHandleW(dllName);
    if (NULL == dynamic_library)
    {
        assert(ERROR_MOD_NOT_FOUND == GetLastError());

        dynamic_library = LoadLibraryW(dllName);
        assert(NULL != dynamic_library);
    }

    decltype(DllGetClassObject) *const pfn_dll_get_class_object = reinterpret_cast<decltype(DllGetClassObject) *>(GetProcAddress(dynamic_library, "DllGetClassObject"));

    if (NULL == pfn_dll_get_class_object)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    IClassFactory *class_factory = NULL;
    HRESULT hr_dll_get_class_object = pfn_dll_get_class_object(rclsid, IID_PPV_ARGS(&class_factory));
    if (!SUCCEEDED(hr_dll_get_class_object))
    {
        return hr_dll_get_class_object;
    }

    HRESULT hr_class_factory_create_instance = class_factory->CreateInstance(nullptr, riid, ppv);
    class_factory->Release();

    // TODO: DllCanUnloadNow

    return hr_class_factory_create_instance;
}

extern "C" bool brx_wsi_get_open_file_name(int filter_count, char const *const *filter_names, char const *const *filter_specs, int *inout_filter_index, void *out_file_name_std_string, void (*std_string_assign_callback)(void *out_file_name_std_string, char const *s))
{
    mcrt_wstring file_name_utf16;
    bool open_file = false;
    {
        HRESULT res_co_initialize_ex = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        assert(SUCCEEDED(res_co_initialize_ex));

        IFileOpenDialog *file_open_dialog = NULL;
        // HRESULT res_co_create_instance = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&file_open_dialog));
        HRESULT res_co_create_instance = NoRegCoCreate(L"Comdlg32.dll", CLSID_FileOpenDialog, IID_PPV_ARGS(&file_open_dialog));
        assert(SUCCEEDED(res_co_create_instance));

        FILEOPENDIALOGOPTIONS file_open_dialog_options;
        HRESULT res_file_open_dialog_get_options = file_open_dialog->GetOptions(&file_open_dialog_options);
        assert(SUCCEEDED(res_file_open_dialog_get_options));

        file_open_dialog_options |= FOS_FORCEFILESYSTEM;
        file_open_dialog_options &= (~static_cast<FILEOPENDIALOGOPTIONS>(FOS_ALLOWMULTISELECT));

        HRESULT res_file_open_dialog_set_options = file_open_dialog->SetOptions(file_open_dialog_options);
        assert(SUCCEEDED(res_file_open_dialog_set_options));

        mcrt_vector<mcrt_wstring> filter_names_utf16(static_cast<size_t>(filter_count));
        mcrt_vector<mcrt_wstring> filter_specs_utf16(static_cast<size_t>(filter_count));
        mcrt_vector<COMDLG_FILTERSPEC> filters(static_cast<size_t>(filter_count));
        {
            iconv_t conversion_descriptor = iconv_open("UTF-16LE", "UTF-8");
            assert(((iconv_t)(-1)) != conversion_descriptor);

            for (uint32_t filter_index = 0U; filter_index < filter_count; ++filter_index)
            {
                {
                    mcrt_string src_utf8 = filter_names[filter_index];
                    mcrt_wstring &dst_utf16 = filter_names_utf16[filter_index];

                    assert(dst_utf16.empty());
                    assert(!src_utf8.empty());

                    dst_utf16.resize(src_utf8.size() + 1U);

                    size_t in_bytes_left = sizeof(src_utf8[0]) * src_utf8.size();
                    size_t out_bytes_left = sizeof(dst_utf16[0]) * dst_utf16.size();
                    char *in_buf = src_utf8.data();
                    char *out_buf = reinterpret_cast<char *>(dst_utf16.data());

                    size_t conversion_result = iconv(conversion_descriptor, &in_buf, &in_bytes_left, &out_buf, &out_bytes_left);
                    assert(((size_t)(-1)) != conversion_result);

                    dst_utf16.resize(reinterpret_cast<decltype(&dst_utf16[0])>(out_buf) - dst_utf16.data());
                }

                filters[filter_index].pszName = filter_names_utf16[filter_index].data();

                {
                    mcrt_string src_utf8 = filter_specs[filter_index];
                    mcrt_wstring &dst_utf16 = filter_specs_utf16[filter_index];

                    assert(dst_utf16.empty());
                    assert(!src_utf8.empty());

                    dst_utf16.resize(src_utf8.size() + 1U);

                    size_t in_bytes_left = sizeof(src_utf8[0]) * src_utf8.size();
                    size_t out_bytes_left = sizeof(dst_utf16[0]) * dst_utf16.size();
                    char *in_buf = src_utf8.data();
                    char *out_buf = reinterpret_cast<char *>(dst_utf16.data());

                    size_t conversion_result = iconv(conversion_descriptor, &in_buf, &in_bytes_left, &out_buf, &out_bytes_left);
                    assert(((size_t)(-1)) != conversion_result);

                    dst_utf16.resize(reinterpret_cast<decltype(&dst_utf16[0])>(out_buf) - dst_utf16.data());
                }

                filters[filter_index].pszSpec = filter_specs_utf16[filter_index].data();
            }

            int result = iconv_close(conversion_descriptor);
            assert(-1 != result);
        }

        HRESULT res_file_open_dialog_set_file_types = file_open_dialog->SetFileTypes(filter_count, filters.data());
        assert(SUCCEEDED(res_file_open_dialog_set_file_types));

        HRESULT res_file_open_dialog_set_file_type_index = file_open_dialog->SetFileTypeIndex(std::min(std::max(static_cast<int>(1), (static_cast<int>(*inout_filter_index) + static_cast<int>(1))), (static_cast<int>(filter_count) + static_cast<int>(1))));
        assert(SUCCEEDED(res_file_open_dialog_set_file_type_index));

        HRESULT res_file_open_dialog_show = file_open_dialog->Show(static_cast<HWND>(s_main_window));
        if (SUCCEEDED(res_file_open_dialog_show))
        {
            UINT file_type;
            HRESULT res_file_open_dialog_get_file_type_index = file_open_dialog->GetFileTypeIndex(&file_type);
            assert(SUCCEEDED(res_file_open_dialog_get_file_type_index));
            (*inout_filter_index) = (static_cast<int>(file_type) - 1);

            IShellItem *item;
            HRESULT res_file_open_dialog_get_result = file_open_dialog->GetResult(&item);
            assert(SUCCEEDED(res_file_open_dialog_get_result));

            WCHAR *name;
            HRESULT res_shell_item_get_display_name = item->GetDisplayName(SIGDN_FILESYSPATH, &name);
            assert(SUCCEEDED(res_shell_item_get_display_name));

            assert(file_name_utf16.empty());
            file_name_utf16 = name;

            open_file = true;

            CoTaskMemFree(name);

            item->Release();
        }
        else
        {
            assert(HRESULT_FROM_WIN32(ERROR_CANCELLED) == res_file_open_dialog_show);

            assert(file_name_utf16.empty());
        }

        file_open_dialog->Release();

        CoFreeUnusedLibraries();

        CoUninitialize();
    }

    if (open_file)
    {
        mcrt_string file_name_utf8;
        {
            mcrt_wstring &src_utf16 = file_name_utf16;
            mcrt_string &dst_utf8 = file_name_utf8;

            assert(dst_utf8.empty());
            assert(!src_utf16.empty());

            constexpr size_t const UNI_MAX_UTF8_BYTES_PER_CODE_POINT = 4U;
            dst_utf8.resize(src_utf16.size() * UNI_MAX_UTF8_BYTES_PER_CODE_POINT + 1U);

            size_t in_bytes_left = sizeof(src_utf16[0]) * src_utf16.size();
            size_t out_bytes_left = sizeof(dst_utf8[0]) * dst_utf8.size();
            char *in_buf = reinterpret_cast<char *>(src_utf16.data());
            char *out_buf = dst_utf8.data();

            iconv_t conversion_descriptor = iconv_open("UTF-8", "UTF-16LE");
            assert(((iconv_t)(-1)) != conversion_descriptor);

            size_t conversion_result = iconv(conversion_descriptor, &in_buf, &in_bytes_left, &out_buf, &out_bytes_left);
            assert(((size_t)(-1)) != conversion_result);

            int result = iconv_close(conversion_descriptor);
            assert(-1 != result);

            dst_utf8.resize(reinterpret_cast<decltype(&dst_utf8[0])>(out_buf) - &dst_utf8[0]);
        }

        std_string_assign_callback(out_file_name_std_string, file_name_utf8.c_str());

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
