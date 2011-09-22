/*
 * Copyright 2002-2011 Guillaume Cottenceau and contributors.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */
// hacked to pieces by mpcomplete

#include "readpng.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <algorithm>

void abort_(const char * s, ...)
{
  va_list args;
  va_start(args, s);
  vfprintf(stderr, s, args);
  fprintf(stderr, "\n");
  va_end(args);
  abort();
}

void PngImage::from_ximage(XImage* image, int w, int h)
{
  alloc(w, h);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      long pixel = XGetPixel(image, x, y);
      png_byte* byte = get(x, y);
      byte[0] = (pixel >> 16) & 0xff;
      byte[1] = (pixel >> 8) & 0xff;
      byte[2] = (pixel) & 0xff;
    }
  }
}

void PngImage::read_file(const std::string& filename)
{
  unsigned char header[8];    // 8 is the maximum size that can be checked
  int x, y;
  png_structp png_ptr;
  png_infop info_ptr;

  FILE *fp = fopen(filename.c_str(), "rb");
  if (!fp)
    abort_("[read_png_file] File %s could not be opened for reading",
           filename.c_str());
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8))
    abort_("[read_png_file] File %s is not recognized as a PNG file");

  /* initialize stuff */
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!png_ptr)
    abort_("[read_png_file] png_create_read_struct failed");

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    abort_("[read_png_file] png_create_info_struct failed");

//  if (setjmp(png_jmpbuf(png_ptr)))
//    abort_("[read_png_file] Error during init_io");

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  png_read_info(png_ptr, info_ptr);

  width = png_get_image_width(png_ptr, info_ptr);
  height = png_get_image_height(png_ptr, info_ptr);
  png_byte color_type = png_get_color_type(png_ptr, info_ptr);
  png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  if (bit_depth != 8 || color_type != PNG_COLOR_TYPE_RGB)
    abort_("[read_png_file] expected 8-bit RGB");

  //number_of_passes = png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);


  /* read file */
  if (setjmp(png_jmpbuf(png_ptr)))
    abort_("[read_png_file] Error during read_image");

  alloc(width, height);

  png_read_image(png_ptr, row_pointers);

  fclose(fp);
}

void PngImage::write_file(const std::string& filename)
{
  /* create file */
  FILE *fp = fopen(filename.c_str(), "wb");
  if (!fp)
    abort_("[write_png_file] File %s could not be opened for writing",
           filename.c_str());


  /* initialize stuff */
  png_structp png_ptr;
  png_infop info_ptr;
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!png_ptr)
    abort_("[write_png_file] png_create_write_struct failed");

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    abort_("[write_png_file] png_create_info_struct failed");

  if (setjmp(png_jmpbuf(png_ptr)))
    abort_("[write_png_file] Error during init_io");

  png_init_io(png_ptr, fp);


  /* write header */
  if (setjmp(png_jmpbuf(png_ptr)))
    abort_("[write_png_file] Error during writing header");

  png_set_IHDR(png_ptr, info_ptr, width, height,
               8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  png_write_info(png_ptr, info_ptr);


  /* write bytes */
  if (setjmp(png_jmpbuf(png_ptr)))
    abort_("[write_png_file] Error during writing bytes");

  png_write_image(png_ptr, row_pointers);


  /* end write */
  if (setjmp(png_jmpbuf(png_ptr)))
    abort_("[write_png_file] Error during end of write");

  png_write_end(png_ptr, NULL);

  fclose(fp);
}

#if 0
void process_file(void)
{
  /* Expand any grayscale, RGB, or palette images to RGBA */
  png_set_expand(png_ptr);

  /* Reduce any 16-bits-per-sample images to 8-bits-per-sample */
  png_set_strip_16(png_ptr);

  for (y=0; y<height; y++) {
    png_byte* row = row_pointers[y];
    for (x=0; x<width; x++) {
      png_byte* ptr = &(row[x*3]);
      printf("Pixel at position [ %d - %d ] has RGB values: %d - %d - %d\n",
             x, y, ptr[0], ptr[1], ptr[2]);

      std::swap(ptr[1], ptr[2]);

      /* perform whatever modifications needed, for example to set red value to 0 and green value to the blue one:
         ptr[0] = 0;
         ptr[1] = ptr[2]; */
    }
  }
}

int main(int argc, char **argv)
{
  if (argc != 3)
    abort_("Usage: program_name <file_in> <file_out>");

  PngImage image;
  read_png_file(argv[1], &image);
//  process_file();
//  write_png_file(argv[2]);

  return 0;
}
#endif
