#include "BufferBitmapSource.hpp"

namespace qrviddec {
	BufferBitmapSource::BufferBitmapSource(int inWidth, int inHeight, unsigned char *inBuffer)  {
		width = inWidth; 
		height = inHeight; 
		buffer = inBuffer; 
	}

	BufferBitmapSource::~BufferBitmapSource() {
	}

	int BufferBitmapSource::getWidth() const {
		return width; 
	}

	int BufferBitmapSource::getHeight() const {
		return height; 
	}

	unsigned char *BufferBitmapSource::getRow(int y, unsigned char *row) {
		return buffer + y * width;
	}

	unsigned char *BufferBitmapSource::getMatrix() {
		return buffer; 
	}
}

