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

#ifndef TEXTBOX_H
#define TEXTBOX_H

#ifdef __cplusplus
extern "C" {
#endif

int textbox_create_new();
int textbox_create_init(int x, int y, int w, int h, const char *data);
void textbox_destroy(int id);

void textbox_show(int id);
void textbox_hide(int id);

void textbox_set_position(int id, int x, int y, int w, int h);
void textbox_set_dimensions(int id, int w, int h);
void textbox_set_x(int id, int x);
void textbox_set_y(int id, int y);
void textbox_set_width(int id, int w);
void textbox_set_height(int id, int h);
void textbox_set_value(int id, const char *str);
void textbox_set_opacity(int id, float value);
void textbox_set_type(int id, int type);
void textbox_set_visible(int id, bool visible);

int textbox_get_x(int id);
int textbox_get_y(int id);
int textbox_get_width(int id);
int textbox_get_height(int id);
const char *textbox_get_value(int id);
float textbox_get_opacity(int id);
int textbox_get_type(int id);
bool textbox_get_visible(int id);
	
#ifdef __cplusplus
}
#endif

#endif
