#ifndef CAT_QR_CODE_PROCESSOR_H
#define CAT_QR_CODE_PROCESSOR_H

#ifdef __cplusplus
extern "C" {
#endif

// Takes luminance raster and dumps decoded string into <512 byte buffer
// NOTE This does not work with RGBA data - It must be monochrome luminance
void qr_process(const unsigned char *raster, int width, int height, char result[512]);
void qr_process_base64_image(const char *b64image, char text[512]);

// Outputs RGB PNG image
// Free the buffer with free() when done
char *qr_generate_base64_image(const char *text, int *width, int *height);

#ifdef __cplusplus
}
#endif

#endif // CAT_QR_CODE_PROCESSOR_H

