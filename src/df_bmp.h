#pragma once


#ifdef __cplusplus
extern "C"
{
#endif


struct DfBitmap;


DfBitmap *LoadBmp(char const *filename);
bool SaveBmp(DfBitmap *bitmap, char const *filename);


#ifdef __cplusplus
}
#endif