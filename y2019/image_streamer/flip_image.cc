#include "flip_image.h"

#define cimg_display 0
#define cimg_use_jpeg
#define cimg_plugin "plugins/jpeg_buffer.h"
#include "third_party/cimg/CImg.h"

void flip_image(const char *input, const int input_size, JOCTET *buffer,
                unsigned int *buffer_size) {
  ::cimg_library::CImg<unsigned char> image;
  image.load_jpeg_buffer((JOCTET *)(input), input_size);
  image.mirror("xy");

  image.save_jpeg_buffer(buffer, *buffer_size, 80);
}