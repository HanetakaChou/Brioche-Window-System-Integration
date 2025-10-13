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

#ifndef _BRX_WSI_H_
#define _BRX_WSI_H_ 1

extern "C" void brx_wsi_init_connection();

extern "C" void brx_wsi_uninit_connection();

extern "C" void *brx_wsi_get_connection();

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
    int window_height);

extern "C" void brx_wsi_uninit_main_window();

extern "C" void brx_wsi_show_main_window();

extern "C" void *brx_wsi_get_main_window();

extern "C" void *brx_wsi_create_image_window(char const *window_name);

extern "C" void brx_wsi_destroy_image_window(void *window);

extern "C" void brx_wsi_present_image_window(void *window, void const *image_buffer, int image_width, int image_height);

extern "C" bool brx_wsi_wait_window();

#endif
