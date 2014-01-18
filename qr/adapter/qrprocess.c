#include <stdlib.h>
#include <stdint.h>

#include "quirc/quirc.h"
#include "libqrencode/qrencode.h"

#include "core/image_loader.h"
#include "core/image_writer.h"

// NOT THREADSAFE
static struct quirc *m_qr = 0;

void qr_process(const unsigned char *buffer, int width, int height, char text[512]) {
	unsigned char *image;
	int w, h, num_codes, ii;

	text[0] = 0;

	if (!m_qr) {
		m_qr = quirc_new();
		if (!m_qr) {
			LOG("{qr} ERROR: Unable to allocate new QR object!");
			return;
		}
	}

	// Resize workspace
	if (quirc_resize(m_qr, width, height) < 0) {
		LOG("{qr} ERROR: Unable to resize to w=%d h=%d", width, height);
		return;
	}

	image = quirc_begin(m_qr, &w, &h);

	if (w != width || h != height) {
		LOG("{qr} ERROR: Width and height do not match w=%d/%d h=%d/%d", w, width, h, height);
		return;
	}

	// Copy image into place
	memcpy(image, buffer, width * height);

	quirc_end(m_qr);

	// Count number of codes
	num_codes = quirc_count(m_qr);

	for (ii = 0; ii < num_codes; ++ii) {
		struct quirc_code code;
		struct quirc_data data;
		quirc_decode_error_t err;

		quirc_extract(m_qr, ii, &code);

		err = quirc_decode(&code, &data);
		if (err) {
			LOG("{qr} ERROR: %s", quirc_strerror(err));
		} else {
			LOG("{qr} Decoded: %s", data.payload);

			strncpy(text, (const char*)data.payload, 512);
			text[511] = 0;
		}
	}
}

void qr_process_base64_image(const char *b64image, char text[512]) {
	int width, height, channels, x, y;
	unsigned char *image = load_image_from_base64(b64image, &width, &height, &channels);

	if (image) {
		LOG("{qr} Successfully decoded image data with width=%d, height=%d, channels=%d", width, height, channels);
	} else {
		LOG("{qr} WARNING: Unable to load image from memory");
	}

	// Default is empty string
	text[0] = 0;

	if (image && width > 0 && height > 0 && channels > 0) {
		if (channels == 1) {
			LOG("{qr} QR processing provided monochrome luminance image");

			qr_process((const unsigned char *)image, width, height, text);
		} else if (channels == 3) {
			LOG("{qr} Processing RGB/BGR input data to luminance raster");

			// Convert to luminance
			const unsigned char *input = (const unsigned char *)image;
			unsigned char *output = image;
			for (y = 0; y < height; ++y) {
				for (x = 0; x < width; ++x) {
					// Green is 2x as intense as other colors, assume RGB or BGR
					int mag = (unsigned int)input[0] + ((unsigned int)input[1] << 1) + (unsigned int)input[2];
					mag /= 4;
					//if (mag < 0) mag = 0;
					if (mag > 255) mag = 255;
					*output++ = (unsigned char)mag;
					input += 3;
				}
			}

			LOG("{qr} QR processing luminance image");

			qr_process((const unsigned char *)image, width, height, text);
		} else if (channels == 4) {
			LOG("{qr} Processing RGBA-type input data to luminance raster");

			// Convert to luminance
			const unsigned char *input = (const unsigned char *)image;
			unsigned char *output = image;
			for (y = 0; y < height; ++y) {
				for (x = 0; x < width; ++x) {
					// Green is 2x as intense as other colors, assume RGBA or BGRA and ignore alpha (experimentally true)
					int mag = (unsigned int)input[0] + ((unsigned int)input[1] << 1) + (unsigned int)input[2];
					mag /= 4;
					//if (mag < 0) mag = 0;
					if (mag > 255) mag = 255;
					*output++ = (unsigned char)mag;
					input += 4;
				}
			}

			LOG("{qr} QR processing luminance image");

			qr_process((const unsigned char *)image, width, height, text);
		}
	}

	LOG("{qr} Result is: '%s'", text);

	free(image);
}

char *qr_generate_base64_image(const char *text, int *width, int *height) {
	int i, j, kk;

	if (!text || !width || !height) {
		LOG("{qr} qr_generate_base64_image invalid input");
		return 0;
	}

	QRcode *qr = QRcode_encodeString(text, 0, QR_ECLEVEL_H, QR_MODE_8, 1);
	if (!qr) {
		LOG("{qr} Unable to encode text %s", text);
		return 0;
	}

	const int IMAGE_SCALE = 16;
	const int image_width = (qr->width + 2) * IMAGE_SCALE;

	// Write out the QR code monochrome image
	// The orientation of the code is important to preserve, and this does it the right way up
	const int image_size = 3 * image_width * image_width;
	unsigned char *image = (unsigned char *)malloc(image_size);
	const int stride = 3 * (qr->width + 2);
	unsigned char *pixel = image;

	// Set image to white
	memset(image, 0xff, image_size);
	
	// Skip first row
	pixel += stride * IMAGE_SCALE * IMAGE_SCALE;
	
	for (i = 0; i < qr->width; ++i) {
		unsigned char *scanline = pixel;

		// Skip left edge
		scanline += 3 * IMAGE_SCALE;

		// Orient properly
		for (j = qr->width - 1; j >= 0; --j) {
			if (qr->data[(j * qr->width) + i] & 0x1) {
				uint64_t *out = (uint64_t *)scanline;

				for (kk = 0; kk < IMAGE_SCALE; ++kk) {
					// Count of these assignments depends on IMAGE_SCALE
					out[0] = 0;
					out[1] = 0;
					out[2] = 0;
					out[3] = 0;
					out[4] = 0;
					out[5] = 0;
					out += stride * IMAGE_SCALE / 8;
				}
			}

			scanline += 3 * IMAGE_SCALE;
		}

		pixel += stride * IMAGE_SCALE * IMAGE_SCALE;
	}

	char *b64image = write_image_to_base64("PNG", image, image_width, image_width, 3);

	if (!b64image) {
		LOG("{qr} Unable to write image wh=%d", qr->width);
	}

	*width = qr->width * IMAGE_SCALE;
	*height = qr->width * IMAGE_SCALE;

	free(image);

	QRcode_free(qr);

	return b64image;
}

