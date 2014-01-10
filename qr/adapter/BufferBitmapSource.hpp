#include <zxing/LuminanceSource.h>

namespace qrviddec {
	using namespace zxing; 

	class BufferBitmapSource : public LuminanceSource {
		private:
			int width, height; 
			const unsigned char * buffer;

		public:
			BufferBitmapSource(int inWidth, int inHeight, const unsigned char *inBuffer);
			virtual ~BufferBitmapSource();

			virtual ArrayRef<char> getRow(int y, ArrayRef<char> row) const;
			virtual ArrayRef<char> getMatrix() const;
		
			virtual bool isCropSupported() const;
			virtual Ref<LuminanceSource> crop(int left, int top, int width, int height) const;
		
			virtual bool isRotateSupported() const;
		
			virtual Ref<LuminanceSource> invert() const;
		
			virtual Ref<LuminanceSource> rotateCounterClockwise() const;
	};
}
