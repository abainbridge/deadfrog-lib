void HLineUnclipped(DfBitmap *bmp, int x, int y, unsigned len, DfColour c)
{
    DfColour * __restrict row = GetLine(bmp, y) + x;
    if (c.a == 255)
    {
#ifdef _MSC_VER
        __stosd((unsigned long*)row, c.c, len);
#else
        for (unsigned i = 0; i < len; i++)
            row[i] = c;
#endif
    }
    else
    {
#if 1 //_M_IX86_FP < 1
        // SSE2 is not available...
        unsigned crb = (c.c & 0xff00ff) * c.a;
        unsigned cg = c.g * c.a;

        unsigned char invA = 255 - c.a;
        for (unsigned i = 0; i < len; i++)
        {
            DfColour *pixel = row + i;
            unsigned rb = (pixel->c & 0xff00ff) * invA + crb;
            unsigned g = pixel->g * invA + cg;

            pixel->c = rb >> 8;
            pixel->g = g >> 8;
        }
#else
        // SSE2 is available...

        // If there's an odd length run, do one pixel slowly...
        if (len & 1)
        {
            unsigned crb = (c.c & 0xff00ff) * c.a;
            unsigned cg = c.g * c.a;

            unsigned char invA = 255 - c.a;
            unsigned rb = (row->c & 0xff00ff) * invA + crb;
            unsigned g = row->g * invA + cg;

            row->c = rb >> 8;
            row->g = g >> 8;

            x++;
            len--;
        }

        // Process any remaining pixels two at a time using SSE4.
        if (len > 0)
        {
            int invAlpha = 256 - c.a;
            __m128i zero = _mm_setzero_si128();

            __m128i colourTimesAlpha = _mm_set1_epi32(c.c & 0xffffff);
            colourTimesAlpha = _mm_unpacklo_epi8(colourTimesAlpha, zero);
            __m128i colourTimesAlphaMull = _mm_set1_epi16((short)c.a);
            colourTimesAlpha = _mm_mullo_epi16(colourTimesAlpha, colourTimesAlphaMull);

            __m128i mullInvAlpha = _mm_set1_epi16(invAlpha);

            mullInvAlpha.m128i_i16[3] = 256;
            mullInvAlpha.m128i_i16[7] = 256;

            int x = 0;
            while (len > 1)
            {
//                __m128i source = _mm_cvtsi64_si128(*(((__int64*)row) + x)); // Load a 64-bit int into a 128-bit reg
                __m128i source = _mm_load_sd((double*)row + x); // Load a 64-bit int into a 128-bit reg
                source = _mm_unpacklo_epi8(source, zero); // Unpack 8-bit components to 16-bit

                __m128i tmp1 = _mm_mullo_epi16(source, mullInvAlpha);    // source * invAlpha
                tmp1 = _mm_adds_epu16(tmp1, colourTimesAlpha);
                tmp1 = _mm_srli_epi16(tmp1, 8);

                source = _mm_packus_epi16(tmp1, tmp1);

                *(((__int64*)row) + x) = _mm_cvtsi128_si64(source);

                x++;
                len -= 2;
            }
        }
#endif
    }
}
