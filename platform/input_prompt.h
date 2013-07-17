/* @license
 * This file is part of the Game Closure SDK.
 *
 * The Game Closure SDK is free software: you can redistribute it and/or modify
 * it under the terms of the Mozilla Public License v. 2.0 as published by Mozilla.
 
 * The Game Closure SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Mozilla Public License v. 2.0 for more details.
 
 * You should have received a copy of the Mozilla Public License v. 2.0
 * along with the Game Closure SDK.  If not, see <http://mozilla.org/MPL/2.0/>.
 */

#ifndef INPUT_PROMPT_H
#define INPUT_PROMPT_H

int input_prompt_show(const char *title, const char *message, const char *value, bool auto_show_keyboard, bool is_password);
void input_prompt_show_soft_keyboard(const char *curr_val, const char *hint, bool has_backward, bool has_foward, const char *input_type, int max_length);
void input_prompt_hide_soft_keyboard();
void input_prompt_show_status_bar();
void input_prompt_hide_status_bar();
#endif
