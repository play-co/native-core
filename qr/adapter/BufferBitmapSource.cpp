#include "BufferBitmapSource.hpp"

namespace qrviddec {
	BufferBitmapSource::BufferBitmapSource(int inWidth, int inHeight, const unsigned char *inBuffer)
	: LuminanceSource(inWidth, inHeight) {
		width = inWidth; 
		height = inHeight; 
		buffer = inBuffer; 
	}

	BufferBitmapSource::~BufferBitmapSource() {
	}

	ArrayRef<char> BufferBitmapSource::getRow(int y, ArrayRef<char> row) const {
		return ArrayRef<char>( (char *)buffer + height * y, width );
	}

	ArrayRef<char> BufferBitmapSource::getMatrix() const {
		return ArrayRef<char>( (char *)buffer, width * height );
	}
	
	bool BufferBitmapSource::isCropSupported() const {
		return false;
	}

	Ref<LuminanceSource> BufferBitmapSource::crop(int left, int top, int width, int height) const {
		return Ref<LuminanceSource> ( 0 );
	}
	
	bool BufferBitmapSource::isRotateSupported() const {
		return false;
	}
	
	Ref<LuminanceSource> BufferBitmapSource::invert() const {
		return Ref<LuminanceSource> ( 0 );
	}
	
	Ref<LuminanceSource> BufferBitmapSource::rotateCounterClockwise() const {
		return Ref<LuminanceSource> ( 0 );
	}
}

