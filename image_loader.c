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

#define TEXTURE_LOAD_ERROR 0


unsigned char *load_image_from_memory(unsigned char *bits, long bits_length, int *width, int *height, int *channels) {
	// must have at least 8 bytes be read
	if (bits_length >= 8) {
		/* Test if it is a png first */
		unsigned char header[8];
		memcpy(header, bits, 8);
		int is_png = !png_sig_cmp(header, 0, 8);

		if (is_png) {
			return load_png_from_memory(bits, bits_length, width, height, channels);
		} else {
			return load_jpg_from_memory(bits, bits_length, width, height, channels);
		}
	}
	return NULL;
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
                                   png_const_charp msg)
{
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
	int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
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
struct my_error_mgr {
	struct jpeg_error_mgr pub;  /* "public" fields */

	jmp_buf setjmp_buffer;  /* for return to caller */
};

typedef struct my_error_mgr *my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
my_error_exit(j_common_ptr cinfo) {
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;
	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message)(cinfo);
	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

unsigned char *load_jpg_from_memory(unsigned char *bits, long bits_length, int *width, int *height, int *channels) {
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	jpeg_create_decompress(&cinfo);
	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		return 0;
	}

	jpeg_mem_src(&cinfo, bits, bits_length);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);
	JSAMPARRAY buffer;      /* Output row buffer */
	int row_stride;
	row_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
	         ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */
	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	*channels = cinfo.output_components;
	*width = cinfo.output_width;
	*height = cinfo.output_height;
	unsigned char *outBits = (unsigned char *) malloc(row_stride * cinfo.output_height);
	int pos = 0;

	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */
		memcpy(outBits + row_stride * (pos), buffer[0], row_stride);
		pos++;
	}

	/* Step 7: Finish decompression */
	(void) jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */
	/* Step 8: Release JPEG decompression object */
	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);
	/* After finish_decompress, we can close the input file.
	 * Here we postpone it until after no more JPEG errors are possible,
	 * so as to simplify the setjmp error logic above.  (Actually, I don't
	 * think that jpeg_destroy can do an error exit, but why assume anything...)
	 */
	return outBits;
}

/* Read JPEG image from a memory segment */
void jpg_init_source(j_decompress_ptr cinfo) {}
boolean jpg_fill_input_buffer(j_decompress_ptr cinfo) {
	// ERREXIT(cinfo, JERR_INPUT_EMPTY);
	return TRUE;
}
void jpg_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
	struct jpeg_source_mgr *src = (struct jpeg_source_mgr *) cinfo->src;

	if (num_bytes > 0) {
		src->next_input_byte += (size_t) num_bytes;
		src->bytes_in_buffer -= (size_t) num_bytes;
	}
}
void jpg_term_source(j_decompress_ptr cinfo) {}
void jpeg_mem_src(j_decompress_ptr cinfo, void *buffer, long nbytes) {
	struct jpeg_source_mgr *src;

	if (cinfo->src == NULL) {   /* first time for this JPEG object? */
		cinfo->src = (struct jpeg_source_mgr *)
		             (*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT,
		                                        sizeof(struct jpeg_source_mgr));
	}

	src = (struct jpeg_source_mgr *) cinfo->src;
	src->init_source = jpg_init_source;
	src->fill_input_buffer = jpg_fill_input_buffer;
	src->skip_input_data = jpg_skip_input_data;
	src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->term_source = jpg_term_source;
	src->bytes_in_buffer = nbytes;
	src->next_input_byte = (JOCTET *)buffer;
}
