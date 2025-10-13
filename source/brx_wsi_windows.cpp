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
#include <cstdlib>
#include <cstring>
#include <cassert>
#include "../../Brioche-ImGui/imgui.h"
#include "../../McRT-Malloc/include/mcrt_string.h"
#include "../../libiconv/include/iconv.h"
#include "../build-windows/resource.h"

struct internal_brx_window
{
    HWND window;
    HDC device_context;
    HDC memory_device_context;
    HBITMAP bitmap;
    int32_t window_width;
    int32_t window_height;
};

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

static void (*s_main_window_key_press_handler)(int key, bool shift_key, bool caps_key, bool ctrl_key, bool alt_key) = NULL;
static void (*s_main_window_key_release_handler)(int key, bool shift_key, bool caps_key, bool ctrl_key, bool alt_key) = NULL;
static void (*s_main_window_button_press_handler)(int button, int x, int y) = NULL;
static void (*s_main_window_button_release_handler)(int button, int x, int y) = NULL;
static void (*s_main_window_scroll_up_handler)(int x, int y) = NULL;
static void (*s_main_window_scroll_down_handler)(int x, int y) = NULL;
static void (*s_main_window_motion_handler)(int x, int y, bool left_button, bool middle_button, bool right_button) = NULL;
static void (*s_main_window_resize_handler)(int width, int height) = NULL;

extern "C" void brx_wsi_init_main_window(
    char const *window_name,
    void (*key_press_handler)(int key, bool shift_key, bool caps_key, bool ctrl_key, bool alt_key),
    void (*key_release_handler)(int key, bool shift_key, bool caps_key, bool ctrl_key, bool alt_key),
    void (*button_press_handler)(int button, int x, int y),
    void (*button_release_handler)(int button, int x, int y),
    void (*scroll_up_handler)(int x, int y),
    void (*scroll_down_handler)(int x, int y),
    void (*motion_handler)(int x, int y, bool left_button, bool middle_button, bool right_button),
    void (*resize_handler)(int width, int height),
    int window_width,
    int window_height)
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

        s_main_window_motion_handler(window_x, window_y, leff_button, middle_button, right_button);
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

        s_main_window_key_press_handler(key, shift_key, caps_key, ctrl_key, alt_key);
    }
        return 0;
    case WM_KEYUP:
    {
        int const key = ImGui_ImplWin32_KeyEventToImGuiKey(wParam, lParam);

        bool shift_key = (0 != (GetKeyState(VK_SHIFT) & 0X8000));
        bool ctrl_key = (0 != (GetKeyState(VK_CONTROL) & 0X8000));
        bool alt_key = (0 != (GetKeyState(VK_MENU) & 0X8000));
        bool caps_key = (0 != (GetKeyState(VK_CAPITAL) & 1));

        s_main_window_key_release_handler(key, shift_key, caps_key, ctrl_key, alt_key);
    }
        return 0;
    case WM_SIZE:
    {
        WORD const new_width = LOWORD(lParam);
        WORD const new_height = HIWORD(lParam);

        assert((0U == new_width) || (new_width == s_main_window_width));
        assert((0U == new_height) || (new_height == s_main_window_height));

        s_main_window_resize_handler(new_width, new_height);
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
    HDC memory_device_context;
    HBITMAP bitmap;
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

    internal_brx_wsi_image_window *unwrapped_window = new (unwrapped_window_base) internal_brx_wsi_image_window{NULL, NULL, NULL, NULL, 0, 0};
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

    assert(NULL == unwrapped_window->memory_device_context);
    unwrapped_window->memory_device_context = CreateCompatibleDC(unwrapped_window->device_context);
    assert(NULL != unwrapped_window->memory_device_context);

    assert(NULL == unwrapped_window->bitmap);
    unwrapped_window->bitmap = CreateCompatibleBitmap(unwrapped_window->device_context, k_window_width, k_window_height);
    assert(NULL != unwrapped_window->bitmap);

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

    assert(NULL != unwrapped_window->bitmap);
    BOOL result_delete_object = DeleteObject(unwrapped_window->bitmap);
    assert(FALSE != result_delete_object);
    unwrapped_window->bitmap = NULL;

    assert(NULL != unwrapped_window->memory_device_context);
    BOOL result_delete_dc = DeleteDC(unwrapped_window->memory_device_context);
    assert(FALSE != result_delete_dc);
    unwrapped_window->memory_device_context = NULL;

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
    assert(NULL != unwrapped_window->memory_device_context);

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

        assert(NULL != unwrapped_window->bitmap);
        BOOL result_delete_object = DeleteObject(unwrapped_window->bitmap);
        assert(FALSE != result_delete_object);
        unwrapped_window->bitmap = NULL;

        assert(NULL == unwrapped_window->bitmap);
        unwrapped_window->bitmap = CreateCompatibleBitmap(unwrapped_window->device_context, image_width, image_height);
        assert(NULL != unwrapped_window->bitmap);

        unwrapped_window->window_width = image_width;
        unwrapped_window->window_height = image_height;
    }

    {
        // write "texture" into "back buffer"
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = image_width;
        bmi.bmiHeader.biHeight = -image_height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        int result_set_dib_bits = SetDIBits(unwrapped_window->device_context, unwrapped_window->bitmap, 0, image_height, image_buffer, &bmi, DIB_RGB_COLORS);
        assert(result_set_dib_bits > 0);
    }

    {
        HBITMAP old_bitmap = reinterpret_cast<HBITMAP>(SelectObject(unwrapped_window->memory_device_context, unwrapped_window->bitmap));
        assert(NULL != old_bitmap && HGDI_ERROR != old_bitmap);

        // copy from "back-buffer" into "front buffer"
        int result_bit_blt = BitBlt(unwrapped_window->device_context, 0, 0, image_width, image_height, unwrapped_window->memory_device_context, 0, 0, SRCCOPY);
        assert(0 != result_bit_blt);

        HGDIOBJ new_bitmap = reinterpret_cast<HBITMAP>(SelectObject(unwrapped_window->memory_device_context, old_bitmap));
        assert(new_bitmap == unwrapped_window->bitmap);
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

extern "C" bool brx_wsi_wait_window()
{
    bool running = true;

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

    return running;
}
