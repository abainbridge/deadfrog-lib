#ifndef INCLUDED_BMP_H
#define INCLUDED_BMP_H


struct BitmapRGBA;


BitmapRGBA *LoadBmp(char const *filename);
bool SaveBmp(BitmapRGBA *bitmap, char const *filename);


#endif
