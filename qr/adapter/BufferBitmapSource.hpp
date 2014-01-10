#include <zxing/LuminanceSource.h>

namespace qrviddec {
	using namespace zxing; 

	class BufferBitmapSource : public LuminanceSource {
		private:
			int width, height; 
			unsigned char * buffer; 

		public:
			BufferBitmapSource(int inWidth, int inHeight, unsigned char * inBuffer); 
			~BufferBitmapSource(); 

			int getWidth() const; 
			int getHeight() const; 
			unsigned char* getRow(int y, unsigned char* row); 
			unsigned char* getMatrix(); 
	}; 
}


