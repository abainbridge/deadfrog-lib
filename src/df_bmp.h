#ifndef INCLUDED_BMP_H
#define INCLUDED_BMP_H


struct DfBitmap;


DfBitmap *LoadBmp(char const *filename);
bool SaveBmp(DfBitmap *bitmap, char const *filename);


#endif
