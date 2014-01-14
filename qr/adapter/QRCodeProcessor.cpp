#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <string>

#include "zxing/qrcode/QRCodeReader.h"
#include "zxing/Exception.h"
#include "zxing/common/GlobalHistogramBinarizer.h"
#include "zxing/DecodeHints.h"

#include "libqrencode/qrencode.h"

#include "BufferBitmapSource.hpp"

#include "core/image_loader.h"
#include "core/image_writer.h"

using namespace std;
using namespace zxing;
using namespace zxing::qrcode;
using namespace qrviddec;

extern "C" void qr_process(const unsigned char *buffer, int width, int height, char text[512]) {
	text[0] = 0;

	try {
		// Convert the buffer to something that the library understands.
		Ref<LuminanceSource> source (new BufferBitmapSource(width, height, buffer));

		// Turn it into a binary image.
		Ref<Binarizer> binarizer (new GlobalHistogramBinarizer(source));
		Ref<BinaryBitmap> image(new BinaryBitmap(binarizer));

		// Tell the decoder to try as hard as possible.
		DecodeHints hints(DecodeHints::QR_CODE_HINT);
		hints.setTryHarder(true);

		// Perform the decoding.
		QRCodeReader reader;
		Ref<Result> result(reader.decode(image, hints));

		// Output the result.
		const char *cstr = result->getText()->getText().c_str();

		for (int ii = 0; ii < 511; ++ii) {
			char ch = cstr[ii];
			text[ii] = ch;
			if (!ch) {
				break;
			}
		}
		text[511] = 0;
	} catch (zxing::Exception& e) {
		cerr << "{qr} ERROR: " << e.what() << endl;
	}
}

extern "C" void qr_process_base64_image(const char *b64image, char text[512]) {
	int width, height, channels;
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
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
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
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
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

extern "C" char *qr_generate_base64_image(const char *text, int *width, int *height) {
	if (!text || !width || !height) {
		LOG("{qr} qr_generate_base64_image invalid input");
		return 0;
	}

	QRcode *qr = QRcode_encodeString8bit(text, 0, QR_ECLEVEL_Q);
	if (!qr) {
		LOG("{qr} Unable to encode text %s", text);
		return 0;
	}

	const int image_width = (qr->width + 2) * 8;

	// Write out the QR code monochrome image
	// The orientation of the code is important to preserve, and this does it the right way up
	const int image_size = 3 * image_width * image_width;
	unsigned char *image = (unsigned char *)malloc(image_size);
	const int stride = 3 * (qr->width + 2);
	unsigned char *pixel = image;

	// Set image to white
	memset(image, 0xff, image_size);
	
	// Skip first row
	pixel += stride * 8 * 8;
	
	for (int i = 0; i < qr->width; ++i) {
		unsigned char *scanline = pixel;

		// Skip left edge
		scanline += 24;
		
		for (int j = 0; j < qr->width; ++j) {
			if (qr->data[(j * qr->width) + i] & 0x1) {
				uint64_t *out = (uint64_t *)scanline;
				for (int kk = 0; kk < 8; ++kk) {
					out[0] = 0;
					out[1] = 0;
					out[2] = 0;
					out += stride;
				}
			}

			scanline += 24;
		}

		pixel += stride * 8 * 8;
	}

	char *b64image = write_image_to_base64("PNG", image, image_width, image_width, 3);

	if (!b64image) {
		LOG("{qr} Unable to write image wh=%d", qr->width);
	}

	*width = qr->width * 8;
	*height = qr->width * 8;

	free(image);

	QRcode_free(qr);

	return b64image;
}

