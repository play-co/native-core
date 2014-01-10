    #include <iostream>
    #include <stdlib.h>
    #include <stdint.h>
    #include <fstream>
    #include <string>
    #include <zxing/qrcode/QRCodeReader.h>
    #include <zxing/Exception.h>
    #include <zxing/common/GlobalHistogramBinarizer.h>
    #include <zxing/DecodeHints.h>
    #include "BufferBitmapSource.h"
     
    using namespace std; 
    using namespace zxing; 
    using namespace zxing::qrcode;
    using namespace qrviddec; 
     
    int main(int argc, char ** argv)
    {
    	try
    	{
     
     
    		// A buffer containing an image. In your code, this would be an image from your camera. In this 
    		// example, it's just an array containing the code for "Hello!". 
    		uint8_t buffer[] = 
    		{ 
    			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    			255,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 255,  0 ,  0 ,  0 ,  0 ,  0 , 255,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 255, 
    			255,  0 , 255, 255, 255, 255, 255,  0 , 255, 255,  0 , 255,  0 , 255, 255,  0 , 255, 255, 255, 255, 255,  0 , 255, 
    			255,  0 , 255,  0 ,  0 ,  0 , 255,  0 , 255, 255,  0 ,  0 , 255, 255, 255,  0 , 255,  0 ,  0 ,  0 , 255,  0 , 255, 
    			255,  0 , 255,  0 ,  0 ,  0 , 255,  0 , 255, 255,  0 ,  0 ,  0 , 255, 255,  0 , 255,  0 ,  0 ,  0 , 255,  0 , 255, 
    			255,  0 , 255,  0 ,  0 ,  0 , 255,  0 , 255, 255, 255,  0 , 255,  0 , 255,  0 , 255,  0 ,  0 ,  0 , 255,  0 , 255, 
    			255,  0 , 255, 255, 255, 255, 255,  0 , 255, 255, 255, 255,  0 ,  0 , 255,  0 , 255, 255, 255, 255, 255,  0 , 255, 
    			255,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 255,  0 , 255,  0 , 255,  0 , 255,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 255, 
    			255, 255, 255, 255, 255, 255, 255, 255, 255,  0 ,  0 ,  0 ,  0 ,  0 , 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    			255,  0 ,  0 , 255,  0 ,  0 , 255,  0 , 255, 255,  0 ,  0 , 255,  0 , 255,  0 , 255, 255, 255, 255, 255,  0 , 255, 
    			255,  0 ,  0 ,  0 ,  0 ,  0 , 255, 255,  0 , 255, 255,  0 , 255,  0 , 255,  0 , 255,  0 ,  0 ,  0 , 255, 255, 255, 
    			255, 255, 255, 255, 255, 255, 255,  0 , 255,  0 ,  0 ,  0 , 255, 255,  0 ,  0 , 255,  0 , 255,  0 ,  0 ,  0 , 255, 
    			255, 255, 255,  0 ,  0 , 255, 255, 255,  0 ,  0 ,  0 , 255,  0 , 255, 255, 255, 255, 255, 255,  0 , 255, 255, 255, 
    			255,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 255, 255,  0 , 255, 255, 255, 255,  0 , 255, 255,  0 , 255,  0 , 255, 255, 
    			255, 255, 255, 255, 255, 255, 255, 255, 255,  0 ,  0 , 255,  0 ,  0 , 255, 255, 255, 255,  0 , 255,  0 ,  0 , 255, 
    			255,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 255, 255, 255,  0 , 255,  0 ,  0 ,  0 ,  0 ,  0 , 255,  0 , 255, 255, 255, 
    			255,  0 , 255, 255, 255, 255, 255,  0 , 255, 255,  0 ,  0 ,  0 ,  0 ,  0 , 255,  0 ,  0 ,  0 ,  0 , 255, 255, 255, 
    			255,  0 , 255,  0 ,  0 ,  0 , 255,  0 , 255,  0 , 255, 255,  0 ,  0 , 255,  0 ,  0 , 255,  0 , 255,  0 ,  0 , 255, 
    			255,  0 , 255,  0 ,  0 ,  0 , 255,  0 , 255,  0 , 255,  0 ,  0 ,  0 , 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    			255,  0 , 255,  0 ,  0 ,  0 , 255,  0 , 255, 255,  0 ,  0 , 255, 255, 255,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 255, 
    			255,  0 , 255, 255, 255, 255, 255,  0 , 255,  0 , 255,  0 , 255, 255,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 255, 
    			255,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 , 255,  0 , 255,  0 ,  0 , 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
    			255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
    		}; 
    		int width = 23; 
    		int height = 23; 
     
    		// Convert the buffer to something that the library understands. 
    		Ref<LuminanceSource> source (new BufferBitmapSource(width, height, buffer)); 
     
    		// Turn it into a binary image. 
    		Ref<Binarizer> binarizer (new GlobalHistogramBinarizer(source)); 
    		Ref<BinaryBitmap> image(new BinaryBitmap(binarizer));
     
    		// Tell the decoder to try as hard as possible. 
    		DecodeHints hints(DecodeHints::DEFAULT_HINT); 
    		hints.setTryHarder(true); 
     
    		// Perform the decoding. 
        QRCodeReader reader;
        Ref<Result> result(reader.decode(image, hints));
     
    		// Output the result. 
        cout << result->getText()->getText() << endl;
     
      } 
    	catch (zxing::Exception& e) 
    	{
        cerr << "Error: " << e.what() << endl;
      }
    	return 0; 
    }


