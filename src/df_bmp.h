#pragma once


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct _DfBitmap DfBitmap;


DfBitmap *LoadBmp(char const *filename);
bool SaveBmp(DfBitmap *bmp, char const *filename);


#ifdef __cplusplus
}
#endif