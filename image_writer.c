#include "core/image_writer.h"

#include <openssl/pem.h>
#include "platform/log.h"

/* A BASE-64 ENCODER USING OPENSSL (by Len Schulwitz)
 * Parameter 1: A pointer to the data you want to base-64 encode.
 * Parameter 2: The number of bytes you want encoded.
 * Return: A character pointer to the base-64 encoded data (null-terminated for string output).
 * On linux, compile with "gcc base64_encode.c -o b64enc -lcrypto" and run with "./b64enc".
 * This software has no warranty and is provided "AS IS".  Use at your own risk.
 */

/*This function will Base-64 encode your data.*/
char * base64encode (const void *b64_encode_me, int encode_this_many_bytes){
    BIO *b64_bio, *mem_bio;   //Declare two BIOs.  One base64 encodes, the other stores memory.
    BUF_MEM *mem_bio_mem_ptr; //Pointer to the "memory BIO" structure holding the base64 data.

    b64_bio = BIO_new(BIO_f_base64());  //Initialize our base64 filter BIO.
    mem_bio = BIO_new(BIO_s_mem());  //Initialize our memory sink BIO.
    BIO_push(b64_bio, mem_bio);  //Link the BIOs (i.e. create a filter-sink BIO chain.)
    BIO_set_flags(b64_bio, BIO_FLAGS_BASE64_NO_NL);  //Don't add a newline every 64 characters.

    BIO_write(b64_bio, b64_encode_me, encode_this_many_bytes); //Encode and write our b64 data.
    BIO_flush(b64_bio);  //Flush data.  Necessary for b64 encoding, because of pad characters.

    BIO_get_mem_ptr(mem_bio, &mem_bio_mem_ptr);  //Store address of mem_bio's memory structure.
    BIO_set_close(mem_bio,BIO_NOCLOSE); //Permit access to mem_ptr after BIOs are destroyed.
    BIO_free_all(b64_bio);  //Destroys all BIOs in chain, starting with b64 (i.e. the 1st one).

    (*mem_bio_mem_ptr).data[(*mem_bio_mem_ptr).length] = '\0';  //Adds a null-terminator.

    return (*mem_bio_mem_ptr).data; //Returns base-64 encoded data. (See: "buf_mem_st" struct).
}




char *write_image_to_base64(const char *image_type, unsigned char * data, int width, int height, int channels) {

	int file_type = -1;
	char *base64 = NULL;
	LOG("JARED image type %s", image_type);

	// infer from fileimage_type what the filetype is, it can either by png => (.png) or jpeg => (.jpg, .jpeg)
	// try png first

	if(!strncmp(image_type, "PNG", 3)) {
		LOG("JARED image type is!!!! %s", image_type);
		file_type = IMAGE_TYPE_PNG;

	} else if(!strncmp(image_type, "JPG", 3) || !strncmp(image_type, "JPEG", 4)) {

		file_type = IMAGE_TYPE_JPEG;

	}

	if (file_type == IMAGE_TYPE_PNG) {
		LOG("JARED WRITING A BASE 64 STR");

		base64 = write_png_to_base64(data, width, height, channels);

	} else if (file_type == IMAGE_TYPE_JPEG) {

		base64 = write_jpeg_to_base64(data, width, height, channels);

	}

	return base64;

}

//png helper funcs for writing to memory

/* structure to store PNG image bytes */
struct mem_encode
{
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
		char *base64 = base64encode(state.buffer, state.size);
		free(state.buffer);
		return base64;
	}
}

//TODO: needs to be implemented
char *write_jpeg_to_base64(unsigned char * data, int width, int height, int channels) {

//	// if incoming image has an alpha channel, copy over only rgb to another buffer
//	// make sure to free this second buffer at the end
//	bool reduce_channels = (channels == 4);
//
//	unsigned char *temp_data = NULL;
//	if (reduce_channels) {
//		temp_data = (unsigned char*)malloc(sizeof(unsigned char) * width * height * 3);
//		int temp_data_pos = 0;
//		int i;
//		for (i = 0; i < width * height * channels; i += 4) {
//			temp_data[temp_data_pos++] = data[i];
//			temp_data[temp_data_pos++] = data[i + 1];
//			temp_data[temp_data_pos++] = data[i + 2];
//		}
//	}
//	else {
//		temp_data = data;
//	}
//
//	struct jpeg_compress_struct cinfo;
//	struct jpeg_error_mgr jerr;
//
//	cinfo.err = jpeg_std_error(&jerr);
//	jpeg_create_compress(&cinfo);
//	//jpeg_stdio_dest(&cinfo, outfile);
//	//useful code for saving to memory as opposed saving to the filesys
//	unsigned char *raw_buffer = NULL;
//	unsigned long raw_buffer_size = 0;
//	jpeg_mem_dest(&cinfo, &raw_buffer, &raw_buffer_size);
//
//	cinfo.image_width = width;
//	cinfo.image_height = height;
//	cinfo.input_components = 3;
//	cinfo.in_color_space = JCS_RGB;
//
//	jpeg_set_defaults(&cinfo);
//	// set the quality [0..100]
//	jpeg_set_quality(&cinfo, 100, true);
//	jpeg_start_compress(&cinfo, true);
//
//	JSAMPROW row_pointer;
//	int row_stride = width * 3;
//
//	while (cinfo.next_scanline < cinfo.image_height) {
//		row_pointer = (JSAMPROW) &temp_data[cinfo.next_scanline*row_stride];
//		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
//	}
//
//	// free temp_data if it was used in reducing channels
//	if (reduce_channels) {
//		free(temp_data);
//	}
//
//	jpeg_finish_compress(&cinfo);
//	jpeg_destroy_compress(&cinfo);
//fopen_failed:
//	return raw_buffer;
//
//
//	THIS NEEDS TO BE IMPLEMENTED...
	return NULL;
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

	// if incoming image has an alpha channel, copy over only rgb to another buffer
	// make sure to free this second buffer at the end
	bool reduce_channels = (channels == 4);

	unsigned char *temp_data = NULL;
	if (reduce_channels) {
		temp_data = (unsigned char*)malloc(sizeof(unsigned char) * width * height * 3);
		int temp_data_pos = 0;
		int i;
		for (i = 0; i < width * height * channels; i += 4) {
			temp_data[temp_data_pos++] = data[i];
			temp_data[temp_data_pos++] = data[i + 1];
			temp_data[temp_data_pos++] = data[i + 2];
		}
	}
	else {
		temp_data = data;
	}

	// append filename to path
	int full_path_len = strlen(path) + strlen("/") + strlen(name);
	char *full_path = (char *)malloc(full_path_len);
	memset(full_path, 0, full_path_len);
	sprintf(full_path, "%s%s%s", path, "/", name);

	FILE *outfile;
	if ((outfile = fopen(full_path, "wb")) == NULL) {
		goto fopen_failed;
	}

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);
	//useful code for saving to memory as opposed saving to the filesys
	//unsigned char *raw_buffer = NULL;
	//unsigned long raw_buffer_size = 0;
	//jpeg_mem_dest(&cinfo, &raw_buffer, &raw_buffer_size);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	// set the quality [0..100]
	jpeg_set_quality(&cinfo, 100, true);
	jpeg_start_compress(&cinfo, true);

	JSAMPROW row_pointer;
	int row_stride = width * 3;

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer = (JSAMPROW) &temp_data[cinfo.next_scanline*row_stride];
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}

	did_write = true;

	// free temp_data if it was used in reducing channels
	if (reduce_channels) {
		free(temp_data);
	}

	jpeg_finish_compress(&cinfo);

	fclose(outfile);

	jpeg_destroy_compress(&cinfo);
fopen_failed:
	return did_write;
}

bool write_png_to_file(const char *path, const char *name, unsigned char *data, int width, int height, int channels) {

	bool did_write = false;

	// append path to filename
	int full_path_len = strlen(path) + strlen("/") + strlen(name);
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
