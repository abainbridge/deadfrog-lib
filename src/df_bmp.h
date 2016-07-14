#pragma once


struct DfBitmap;


DfBitmap *LoadBmp(char const *filename);
bool SaveBmp(DfBitmap *bitmap, char const *filename);
