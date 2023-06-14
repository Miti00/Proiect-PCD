#include <stdio.h>

struct BITMAP_header {
  char name[2]; // this should be equal to BM
  unsigned int size;
  int garbage;               // this is not required
  unsigned int image_offset; // offset from where image starts in file.
};

struct DIB_header {
  unsigned int header_size; // size of this header.
  unsigned int width;
  unsigned int height;
  unsigned short int colorplanes;
  unsigned short int bitsperpixel;
  unsigned int compression;
  unsigned int image_size;
  unsigned int temp[4];
};

struct RGB {
  unsigned char blue;
  unsigned char green;
  unsigned char red;
};

struct Image {
  int height;
  int width;
  struct RGB **rgb;
};

struct BMP {
  struct BITMAP_header header;
  struct DIB_header dibheader;
  struct Image image;
};

struct Image readImage(FILE *fp, int height, int width);
void freeImage(struct Image pic);
unsigned char grayscale(struct RGB rgb);
void RGBImageToGrayscale(struct Image pic);
void imageToText(struct Image img, char *filename, char *txt_filename);
int BMPwriteBW(struct BMP *bmp, char *filename, char *bw_filename);
int createBWImage(struct BMP *bmp);
struct BMP *readBMP(char *filename);