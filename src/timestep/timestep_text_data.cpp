/* @license
 * This file is part of the Game Closure SDK.
 *
 * The Game Closure SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * The Game Closure SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with the Game Closure SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "timestep_text_data.h"
#include <stdlib.h>


timestep_text_data *timestep_text_data_init() {
    timestep_text_data *text_data = (timestep_text_data*) malloc(sizeof(timestep_text_data));
    text_data->color.r = 0;
    text_data->color.g = 0;
    text_data->color.b = 0;
    text_data->color.a = 0;

    text_data->background_color.r = 0;
    text_data->background_color.g = 0;
    text_data->background_color.b = 0;
    text_data->background_color.a = 0;

    text_data->horizontal_padding = 0;
    text_data->vertical_padding = 0;
    text_data->line_height = 0;
    text_data->text_align = 0;
    text_data->vertical_align = 0;
    text_data->multiline = true;
    text_data->font_size = 0;
    text_data->font_family = NULL;
    text_data->font_weight = NULL;
    text_data->stroke_style = NULL;
    text_data->line_width = 0;
    text_data->text = NULL;
    text_data->shadow = false;

    text_data->shadow_color.r = 0;
    text_data->shadow_color.g = 0;
    text_data->shadow_color.b = 0;
    text_data->shadow_color.a = 0;

    return text_data;
}

void timestep_text_data_delete(timestep_text_data *text_data) {
    free(text_data->font_family);
    free(text_data->font_weight);
    free(text_data->stroke_style);
    free(text_data->text);
    free(text_data);
}
