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

int input_open_prompt(const char *title, const char *message, const char *ok_text, const char *cancel_text, const char *value, bool auto_show_keyboard, bool is_password);
void input_show_keyboard(const char *curr_val, const char *hint, bool has_backward, bool has_foward, const char *input_type, const char *input_return_button, int max_length, int cursorPos);
void input_hide_keyboard();

#endif
