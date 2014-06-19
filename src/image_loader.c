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

#include "core/image_loader.h"
#include "log.h"
#include <stdlib.h>

#include "core/deps/turbojpeg/turbojpeg.h"

#define TEXTURE_LOAD_ERROR 0


//// Conversion from Base64

#define DC 0

static const unsigned char FROM_BASE64[256] = {
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 0-15
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 16-31
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, 62, DC, DC, DC, 63, // 32-47
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, DC, DC, DC, DC, DC, DC, // 48-63
    DC, 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10, 11, 12, 13, 14, // 64-79
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, DC, DC, DC, DC, DC, // 80-95
    DC, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 96-111
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, DC, DC, DC, DC, DC, // 112-127
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // 128-
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // Extended
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // ASCII
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // Extended
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // ASCII
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // Extended
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, // ASCII
    DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC, DC
};

#undef DC


static int GetBinaryLengthFromBase64Length(const char *encoded_buffer, int bytes) {
    if (bytes <= 0) {
        return 0;
    }

    // Skip characters from end until one is a valid BASE64 character
    while (bytes >= 1) {
        unsigned char ch = encoded_buffer[bytes - 1];

        if (ch == '0' || FROM_BASE64[ch] != 0) {
            break;
        }

        --bytes;
    }

    return (bytes * 3) / 4;
}

static int ReadBase64(const char *encoded_buffer, int encoded_bytes, void *decoded_buffer, int decoded_bytes) {
    // Skip characters from end until one is a valid BASE64 character
    while (encoded_bytes >= 1) {
        unsigned char ch = encoded_buffer[encoded_bytes - 1];

        if (ch == '0' || FROM_BASE64[ch] != 0) {
            break;
        }

        --encoded_bytes;
    }

    if (encoded_bytes <= 0 || decoded_bytes <= 0 ||
            decoded_bytes < (encoded_bytes * 3) / 4) {
        return 0;
    }

    const unsigned char *from = (const unsigned char*)( encoded_buffer );
    unsigned char *to = (unsigned char*)( decoded_buffer );

    unsigned char a, b, c, d;

    int ii, jj, end;
    for (ii = 0, jj = 0, end = encoded_bytes - 3; ii < end; ii += 4, jj += 3) {
        a = FROM_BASE64[from[ii]];
        b = FROM_BASE64[from[ii+1]];
        c = FROM_BASE64[from[ii+2]];
        d = FROM_BASE64[from[ii+3]];

        to[jj] = (a << 2) | (b >> 4);
        to[jj+1] = (b << 4) | (c >> 2);
        to[jj+2] = (c << 6) | d;
    }

    switch (encoded_bytes & 3) {
    case 3: // 3 characters left
        a = FROM_BASE64[from[ii]];
        b = FROM_BASE64[from[ii+1]];
        c = FROM_BASE64[from[ii+2]];

        to[jj] = (a << 2) | (b >> 4);
        to[jj+1] = (b << 4) | (c >> 2);
        return jj+2;

    case 2: // 2 characters left
        a = FROM_BASE64[from[ii]];
        b = FROM_BASE64[from[ii+1]];

        to[jj] = (a << 2) | (b >> 4);
        return jj+1;
    }

    return jj;
}

unsigned short readShort(unsigned char *bits) {
    return (bits[0] << 8) + bits[1];
}

unsigned char *load_image_from_memory(unsigned char *bits, long bits_length, int *width, int *height, int *channels, long *size, int *compression_type) {
    unsigned char *data = NULL;
    *size = 0;
    *compression_type = 0;

    // must have at least 8 bytes be read
    if (bits_length >= 8) {
        /* Test if it is a png first */
        unsigned char header[8];
        memcpy(header, bits, 8);
        int is_png = !png_sig_cmp(header, 0, 8);
        int is_pkm = !is_png && !strncmp("PKM 10", (char*) header, 6);
        int is_jpg = !is_png && !is_pkm && header[0] == 0xFF && header[1] == 0xD8;

        if (is_png) {
            data = load_png_from_memory(bits, bits_length, width, height, channels);
            *size = (*channels) * (*width) * (*height);
        } else if (is_pkm) {
            // ETC HEADER LAYOUT -> 16 bytes total
            // tag -> "PKM 10" 6 bytes
            // format -> 2 bytes
            // texWidth -> 2 bytes
            // texHeight -> 2 bytes
            // originalWidth -> 2 bytes
            // originalHeight -> 2 bytes
            if (bits_length > sizeof(unsigned char) * 16) {
                *compression_type = 36196;// Opengl Constant for GL_ETC1_RGB8_OES (ETC1 compression)
                *channels = 3;
                *width = readShort(bits + 8);
                *height = readShort(bits + 10);
                *size = sizeof(unsigned char) * (bits_length - 16);
                data = (unsigned char*) malloc(*size);
                memcpy(data, bits + 16, *size);
            }
        } else if (is_jpg) {
            data = load_jpg_from_memory(bits, bits_length, width, height, channels);
            *size = (*channels) * (*width) * (*height);
        } else {
            LOG("Unknown image type, skipping load");
        }
    }
    return data;
}

unsigned char *load_image_from_base64(const char *base64image, int *width, int *height, int *channels) {
    int len = (int)strlen(base64image);
    int decoded_bytes = GetBinaryLengthFromBase64Length(base64image, len);

    // Read base64 image into binary image data
    char *decoded_buffer = (char *)malloc(decoded_bytes);
    int read_bytes = ReadBase64(base64image, len, decoded_buffer, decoded_bytes);

    // Load PNG/JPEG image
    long size;
    int compression_type;
    unsigned char *image = load_image_from_memory((unsigned char *)decoded_buffer, read_bytes, width, height, channels, &size, &compression_type);

    free(decoded_buffer);

    return image;
}

struct bounded_buffer {
    unsigned char *pos;
    unsigned char *end;
};

//helper function for load_png_from_memory
void png_image_bytes_read(png_structp png_ptr, png_bytep data, png_size_t length) {
    struct bounded_buffer *buff = (struct bounded_buffer*) png_ptr->io_ptr;
    unsigned char *next_pos = buff->pos + length;
    if (next_pos <= buff->end) {
        memcpy(data, buff->pos , length);
        buff->pos = next_pos;
    }
}

static void readpng2_error_handler(png_structp png_ptr,
                                   png_const_charp msg) {
    jmp_buf *jbuf;

    LOG("{resources} PNG image is corrupted.  Error=%s\n", msg);

    jbuf = png_get_error_ptr(png_ptr);

    longjmp(*jbuf, 1);
}

unsigned char *load_png_from_memory(unsigned char *bits, long bits_length, int *width, int *height, int *channels) {
    jmp_buf jbuf;

    //create png struct
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, &jbuf, readpng2_error_handler, NULL);

    if (!png_ptr) {
        return NULL;
    }

    //create png info struct
    png_infop info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
        return NULL;
    }

    //create png info struct
    png_infop end_info = png_create_info_struct(png_ptr);

    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        return NULL;
    }

    if (setjmp(jbuf)) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return NULL;
    }

    //create a bounded buffer for reading (set the inital pos to 8 <right after the header>)
    struct bounded_buffer buff = {bits + 8, bits + bits_length};
    png_set_read_fn(png_ptr, &buff, png_image_bytes_read);
    //let libpng know you already read the first 8 bytes
    png_set_sig_bytes(png_ptr, 8);
    // read all the info up to the image data
    png_read_info(png_ptr, info_ptr);
    int bit_depth, color_type;
    png_uint_32 twidth, theight;
    // get info about png
    png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type,
                 NULL, NULL, NULL);

    // If color type would be paletted,
    if (color_type & PNG_COLOR_TYPE_PALETTE) {
        // Convert to RGB to be OpenGL compatible
        png_set_palette_to_rgb(png_ptr);
    }

    // If color type would be grayscale and bit depth is low,
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        // Bump it up to be OpenGL compatible
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }

    //update width and height based on png info
    *width = twidth;
    *height = theight;
    // Update the png info struct.
    png_read_update_info(png_ptr, info_ptr);
    *channels = (int)png_get_channels(png_ptr, info_ptr);
    size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    // Allocate the image_data as a big block, to be given to opengl
    unsigned char  *image_data = (unsigned char *) malloc(rowbytes * (*height));

    if (!image_data) {
        //clean up memory and close stuff
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        return NULL;
    }

    //row_pointers is for pointing to image_data for reading the png with libpng
    png_bytep *row_pointers = (png_bytep *)malloc((*height) * sizeof(png_bytep));

    if (!row_pointers) {
        //clean up memory and close stuff
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        free(image_data);
        return NULL;
    }

    // set the individual row_pointers to point at the correct offsets of image_data
    int i;

    for (i = 0; i < *height; ++i) {
        row_pointers[i] = image_data + i * rowbytes;
    }

    //read the png into image_data through row_pointers
    png_read_image(png_ptr, row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    free(row_pointers);
    return image_data;
}

unsigned char *load_jpg_from_memory(unsigned char *bits, long bits_length, int *width, int *height, int *channels) {
    int jpegSubsamp, w, h, pitch;

    tjhandle _jpegDecompressor = tjInitDecompress();

    tjDecompressHeader2(_jpegDecompressor, bits, bits_length, &w, &h, &jpegSubsamp);

    pitch = tjPixelSize[TJPF_RGB] * w;

    unsigned char *buffer = malloc(pitch * h + 8);
    // Add 8 extra bytes to fix bug in tjDecompress where it is writing beyond the end of the buffer

    tjDecompress2(_jpegDecompressor, bits, bits_length, buffer, w, pitch, h, TJPF_RGB, TJFLAG_FASTDCT);

    tjDestroy(_jpegDecompressor);

    *width = w;
    *height = h;
    *channels = 3;
    return buffer;
}
