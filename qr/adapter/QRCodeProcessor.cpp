#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <string>
#include "zxing/qrcode/QRCodeReader.h"
#include "zxing/Exception.h"
#include "zxing/common/GlobalHistogramBinarizer.h"
#include "zxing/DecodeHints.h"
#include "BufferBitmapSource.hpp"

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


