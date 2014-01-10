#ifndef CAT_QR_CODE_PROCESSOR_H
#define CAT_QR_CODE_PROCESSOR_H

#ifdef __cplusplus
extern "C" {
#endif

// Takes luminance raster and dumps decoded string into <512 byte buffer
// NOTE This does not work with RGBA data - It must be monochrome luminance
void qr_process(const unsigned char *buffer, int width, int height, char result[512]);

#ifdef __cplusplus
}
#endif

#endif // CAT_QR_CODE_PROCESSOR_H

