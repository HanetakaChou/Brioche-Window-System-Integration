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
    void *handler_context);

extern "C" void brx_wsi_uninit_main_window();

extern "C" void *brx_wsi_get_main_window();

extern "C" void brx_wsi_get_main_window_size(int *out_window_width, int *out_window_height);

extern "C" void brx_wsi_get_main_window_scale(float *out_window_width_scale, float *out_window_height_scale);

extern "C" void brx_wsi_show_main_window();

extern "C" void brx_wsi_set_main_window_size(int window_width, int window_height);

extern "C" void *brx_wsi_create_image_window(char const *window_name);

extern "C" void brx_wsi_destroy_image_window(void *window);

extern "C" void brx_wsi_present_image_window(void *window, void const *image_buffer, int image_width, int image_height);

extern "C" void brx_wsi_run_main_loop(bool (*tick_callback)(void *tick_context), void *tick_context);

extern "C" bool brx_wsi_get_open_file_name(int filter_count, char const *const *filter_names, char const *const *filter_specs, int *inout_filter_index, void *out_file_name_std_string, void (*std_string_assign_callback)(void *out_file_name_std_string, char const *s));

extern "C" void brx_wsi_tcc();

#endif
