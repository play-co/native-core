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

#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#ifdef UNITY
#include "Unity/png.h"
#else
#include "core/deps/png/png.h"
#include "core/deps/png/pngstruct.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

unsigned char *load_image_from_base64(const char *base64image, int *width, int *height, int *channels);
unsigned char *load_image_from_memory(unsigned char *bits, long bits_length, int *width, int *height, int *channels, long *size, int *compression_type);
unsigned char *load_png_from_memory(unsigned char *bits, long bits_length, int *width, int *height, int *channels);
unsigned char *load_jpg_from_memory(unsigned char *bits, long bits_length, int *width, int *height, int *channels);
//png helper func
void png_image_bytes_read(png_structp png_ptr, png_bytep data, png_size_t length);

#ifdef __cplusplus
}
#endif

#endif
