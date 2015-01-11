#include "core/image_writer.h"

#include "core/deps/turbojpeg/turbojpeg.h"

#include "platform/log.h"

#include <stdlib.h>

/*
	A base64 encoder NOT using OpenSSL (by a very snarky Chris Taylor)

	int GetBase64LengthFromBinaryLength(int bytes)

		Get size of string buffer (neglecting terminating nul character) required to represent
	the given length of data (in bytes).

	void WriteBase64(const void *buffer, int bytes, char *encoded_buffer)

		Write base64 string.  Does not write terminating nul character.

	buffer: Input data bytes
	bytes: Number of bytes of input data
	encoded_buffer: Output data string
	encoded_bytes: Size of output data string (from GetBase64LengthFromBinaryLength)
 */

size_t GetBase64LengthFromBinaryLength(size_t bytes) {
    if (bytes <= 0) return 0;

    return ((bytes + 2) / 3) * 4;
}

static const char *TO_BASE64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void WriteBase64(const void *buffer, size_t bytes, char *encoded_buffer) {
    const unsigned char *data = (const unsigned char *)buffer;

    size_t ii, jj, end;
    for (ii = 0, jj = 0, end = bytes - 2; ii < end; ii += 3, jj += 4) {
        encoded_buffer[jj] = TO_BASE64[data[ii] >> 2];
        encoded_buffer[jj+1] = TO_BASE64[((data[ii] << 4) | (data[ii+1] >> 4)) & 0x3f];
        encoded_buffer[jj+2] = TO_BASE64[((data[ii+1] << 2) | (data[ii+2] >> 6)) & 0x3f];
        encoded_buffer[jj+3] = TO_BASE64[data[ii+2] & 0x3f];
    }

    switch (ii - end) {
    default:
    case 2: // Nothing to write
        break;

    case 1: // Need to write final 1 byte
        encoded_buffer[jj] = TO_BASE64[data[bytes-1] >> 2];
        encoded_buffer[jj+1] = TO_BASE64[(data[bytes-1] << 4) & 0x3f];
        encoded_buffer[jj+2] = '=';
        encoded_buffer[jj+3] = '=';
        break;

    case 0: // Need to write final 2 bytes
        encoded_buffer[jj] = TO_BASE64[data[bytes-2] >> 2];
        encoded_buffer[jj+1] = TO_BASE64[((data[bytes-2] << 4) | (data[bytes-1] >> 4)) & 0x3f];
        encoded_buffer[jj+2] = TO_BASE64[(data[bytes-1] << 2) & 0x3f];
        encoded_buffer[jj+3] = '=';
        break;
    }
}

char *write_image_to_base64(const char *image_type, unsigned char * data, int width, int height, int channels) {
    int file_type = -1;
    char *base64 = NULL;

    // infer from fileimage_type what the filetype is, it can either by png => (.png) or jpeg => (.jpg, .jpeg)
    // try png first

    if(!strncmp(image_type, "PNG", 3)) {
        file_type = IMAGE_TYPE_PNG;
    } else if(!strncmp(image_type, "JPG", 3) || !strncmp(image_type, "JPEG", 4)) {
        file_type = IMAGE_TYPE_JPEG;
    }

    if (file_type == IMAGE_TYPE_PNG) {
        base64 = write_png_to_base64(data, width, height, channels);
    } else if (file_type == IMAGE_TYPE_JPEG) {
        base64 = write_jpeg_to_base64(data, width, height, channels);
    } else {
        LOG("WARNING: Unsupported image type for base64: %s", image_type);
    }

    return base64;
}

//png helper funcs for writing to memory

/* structure to store PNG image bytes */
struct mem_encode {
    char *buffer;
    size_t size;
};

void png_write_data_func(png_structp png_ptr, png_bytep data, png_size_t length) {
    /* with libpng15 next line causes pointer deference error; use libpng12 */
    struct mem_encode* p=(struct mem_encode*)png_get_io_ptr(png_ptr); /* was png_ptr->io_ptr */
    size_t nsize = p->size + length;

    /* allocate or grow buffer */
    if(p->buffer)
        p->buffer = realloc(p->buffer, nsize);
    else
        p->buffer = malloc(nsize);

    if(!p->buffer)
        png_error(png_ptr, "Write Error");

    /* copy new bytes to end of buffer */
    memcpy(p->buffer + p->size, data, length);
    p->size += length;
}

char *write_png_to_base64(unsigned char * data, int width, int height, int channels) {
    struct mem_encode state;
    state.buffer = NULL;
    state.size = 0;

    png_structp png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        goto png_create_write_struct_failed;
    }

    png_infop info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        goto png_create_info_struct_failed;
    }

    // Set up error handling

    if (setjmp (png_jmpbuf (png_ptr))) {
        goto png_failure;
    }

    // Set image attributes
    png_set_IHDR (png_ptr, info_ptr, width, height, 8, channels == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA , PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_byte **row_pointers = (png_byte **) malloc(height * sizeof (png_byte *));

    int i = 0;
    int rowbytes = channels * width;
    for (i = 0; i < height; i++) {
        row_pointers[i] = (unsigned char*)(data + i * rowbytes);
    }

    // Write the image data to fp

    png_set_write_fn(png_ptr, &state, png_write_data_func, NULL);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    // Only free the pointer to the row pointers as row pointers point
    // image data that does not belong to this function
    free (row_pointers);

png_failure:
png_create_info_struct_failed:
    png_destroy_write_struct (&png_ptr, &info_ptr);
png_create_write_struct_failed:

    if (state.buffer == NULL) {
        return NULL;
    } else {
        const size_t b64size = GetBase64LengthFromBinaryLength(state.size);
        char *base64 = malloc(b64size + 1);

        WriteBase64(state.buffer, state.size, base64);
        base64[b64size] = '\0';

        free(state.buffer);
        return base64;
    }
}

char *write_jpeg_to_base64(unsigned char * data, int width, int height, int channels) {
    char *base64 = 0;

    tjhandle _jpegCompressor = tjInitCompress();

    unsigned char *buffer = 0;
    unsigned long buffer_size = 0;

    int retval = tjCompress2(_jpegCompressor, buffer, width, 0, height,
                             channels == 3 ? TJPF_RGB : TJPF_RGBA,
                             &buffer, &buffer_size, TJSAMP_444, 90,
                             TJFLAG_FASTDCT);

    // If failure,
    if (retval != 0 || !buffer) {
        LOG("WARNING: Unable to compress %d x %d base64 JPEG", width, height);
    } else {
        const size_t b64size = GetBase64LengthFromBinaryLength(buffer_size);
        char *base64 = malloc(b64size + 1);

        WriteBase64(buffer, buffer_size, base64);
        base64[b64size] = '\0';
    }

    if (buffer) {
        tjFree(buffer);
    }

    tjDestroy(_jpegCompressor);

    return base64;
}

bool write_image_to_file(const char *path, const char *name, unsigned char * data, int width, int height, int channels) {
    int file_type = -1;
    bool did_write = false;

    // infer from filename what the filetype is, it can either by png => (.png) or jpeg => (.jpg, .jpeg)
    // try png first

    if(strstr(name, ".png")) {

        file_type = IMAGE_TYPE_PNG;

    } else if(strstr(name, ".jpg") || strstr(name, ".jpeg")) {

        file_type = IMAGE_TYPE_JPEG;

    }

    if (file_type == IMAGE_TYPE_PNG) {

        did_write = write_png_to_file(path, name, data, width, height, channels);

    } else if (file_type == IMAGE_TYPE_JPEG) {

        did_write = write_jpeg_to_file(path, name, data, width, height, channels);

    }
    return did_write;
}

bool write_jpeg_to_file(const char *path, const char *name, unsigned char *data, int width, int height, int channels) {
    bool did_write = false;

    // append filename to path
    size_t full_path_len = strlen(path) + strlen("/") + strlen(name);
    char *full_path = (char *)malloc(full_path_len);
    memset(full_path, 0, full_path_len);
    sprintf(full_path, "%s%s%s", path, "/", name);

    FILE *outfile = fopen(full_path, "wb");

    if (!outfile) {
        LOG("WARNING: Unable to open write path: %s", name);
    } else {
        tjhandle _jpegCompressor = tjInitCompress();

        unsigned char *buffer = 0;
        unsigned long buffer_size = 0;

        int retval = tjCompress2(_jpegCompressor, buffer, width, 0, height,
                                 channels == 3 ? TJPF_RGB : TJPF_RGBA,
                                 &buffer, &buffer_size, TJSAMP_444, 90,
                                 TJFLAG_FASTDCT);

        // If failure,
        if (retval != 0 || !buffer) {
            LOG("WARNING: Unable to compress JPEG: %s", name);
        } else {
            fwrite(buffer, buffer_size, 1, outfile);

            did_write = true;
        }

        if (buffer) {
            tjFree(buffer);
        }

        tjDestroy(_jpegCompressor);

        fclose(outfile);
    }

    return did_write;
}

bool write_png_to_file(const char *path, const char *name, unsigned char *data, int width, int height, int channels) {
    bool did_write = false;

    // append path to filename
    size_t full_path_len = strlen(path) + strlen("/") + strlen(name);
    char *full_path = (char *)malloc(full_path_len);
    memset(full_path, 0, full_path_len);
    sprintf(full_path, "%s%s%s", path, "/", name);

    FILE *fp = fopen (full_path, "wb");
    if (! fp) {
        goto fopen_failed;
    }

    png_structp png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        goto png_create_write_struct_failed;
    }

    png_infop info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        goto png_create_info_struct_failed;
    }

    // Set up error handling

    if (setjmp (png_jmpbuf (png_ptr))) {
        goto png_failure;
    }

    // Set image attributes

    png_set_IHDR (png_ptr, info_ptr, width, height, 8, channels == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA , PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_byte **row_pointers = (png_byte **) malloc(height * sizeof (png_byte *));
    int i;
    int rowbytes = channels * width;
    for (i = 0; i < height; i++) {
        row_pointers[i] = data + i * rowbytes;
    }

    // Write the image data to fp
    png_init_io (png_ptr, fp);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    did_write = true;

    // Only free the pointer to the row pointers as row pointers point
    // image data that does not belong to this function
    free (row_pointers);

png_failure:
png_create_info_struct_failed:
    png_destroy_write_struct (&png_ptr, &info_ptr);
png_create_write_struct_failed:
    fclose (fp);
fopen_failed:
    return did_write;
}
