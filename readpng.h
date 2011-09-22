#ifndef _READPNG_H
#define _READPNG_H

#include <string>

#define PNG_DEBUG 3
#include <png.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// We assume all PNGs are non-interlaced 8-bit RGB = 3 bytes per pixel
struct PngImage {
  png_bytep* row_pointers;
  int width, height;

  ~PngImage() {
    for (int y=0; y<height; y++)
      delete row_pointers[y];
    delete row_pointers;
  }
  void alloc(int w, int h) {
    width = w;
    height = h;
    row_pointers = new png_bytep[height];
    for (int y = 0; y < height; ++y)
      row_pointers[y] = new png_byte[width*3];
  }
  void from_ximage(XImage* image, int w, int h);
  void read_file(const std::string& filename);
  void write_file(const std::string& filename);
  png_byte* get(int x, int y) {
    // RGB has 3 bytes per pixel
    return &row_pointers[y][x*3];
  }
  void set(int x, int y, png_byte* rgb) {
    if (x >= width || y >= height)
      return;
    png_byte* dest = get(x, y);
    dest[0] = rgb[0];
    dest[1] = rgb[1];
    dest[2] = rgb[2];
  }
};

#endif // header guard
