// Replaces the MSVC standard library in order to make smaller executables.

#if USE_OWN_LIBC

#include <windows.h>
#include <stdint.h>

void * __cdecl operator new(size_t bytes)
{
    return HeapAlloc(GetProcessHeap(), 0, bytes);
}

void * __cdecl operator new[](size_t bytes)
{
    return HeapAlloc(GetProcessHeap(), 0, bytes);
}

void __cdecl operator delete(void *ptr)
{
    if (ptr) HeapFree(GetProcessHeap(), 0, ptr);
}

void __cdecl operator delete[](void *ptr)
{
    if (ptr) HeapFree(GetProcessHeap(), 0, ptr);
}

extern "C" int __cdecl __purecall(void)
{
    return 0;
}

#define LOWER_CASE_MASK ('a' - 'A')

int isdigit(int c) { return c >= '0' && c <= '9'; }
int isalpha(int c) { c |= LOWER_CASE_MASK; 
    return c >= 'a' && c <= 'z'; }
int isupper(int c) { 
    return !(c & LOWER_CASE_MASK); }

int strncmp(char const *a, char const *b, size_t maxCount) {
    while (*a && maxCount) {
        if (*a != *b)
            return *a - *b;
        a++; b++;
        maxCount--;
    }

    return 0;
}

int stricmp(char const *a, char const *b) {
    while (*a) {
        char diff = (*a - *b) & ~LOWER_CASE_MASK;
        if (diff)
            return diff;
        a++; b++;
    }

    return 0;
}

int strnicmp(char const *a, char const *b, size_t maxCount) {
    while (*a && maxCount) {
        char diff = (*a - *b) & ~LOWER_CASE_MASK;
        if (diff)
            return diff;
        a++; b++;
        maxCount--;
    }

    return 0;
}

int atoi(char const *str) {
    return 12;
}

char *itoa(int val, char *dstBuf, int radix) {
    return NULL;
}

#include <Winuser.h>
int __cdecl vsprintf(char *string, const char *format, va_list ap) {
    return wvsprintf(string, format, ap);
}

#include <math.h>
double _hypot(double x, double y) {
    return sqrt(x*x + y*y);
}

void exit(int code) {
    while (1);
}

#pragma function(memcpy)
void * __cdecl memcpy(void *dst, void const *src, size_t len) {
    for (size_t i = 0; i < len; i++)
        ((char*)dst)[i] = ((char*)src)[i];
    return dst;
}

#pragma function(memset)
void * __cdecl memset(void *target, int value, size_t len) {
    char *p = (char *)target;
    while (len-- > 0)
        *p++ = value;
    return target;
}

static uint64_t g_state = 0x853c49e6748fea9bull;
int rand()
{
    static uint64_t inc = 0xda3e39cb94b95bdbull;
    uint64_t oldstate = g_state;
    g_state = oldstate * 6364136223846793005ull + (inc | 1);
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    int rv = (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    return rv & 0x7fff;
}

void srand(unsigned seed)
{
    g_state = seed;
}

static const float PI_BY_2 = 1.5707963f;
#pragma function(cosf)
float cosf(float x)
{
    float rv = sinf(x + PI_BY_2);
    return rv;
}

#pragma function(sinf)
float sinf(float x)
{   
    const double _PI = 3.14159265358979323846;

//     float start_x = x;
//     float other_x = x;
//     while (other_x < -PI_BY_2)
//         other_x += _PI;
//     while (other_x > PI_BY_2)
//         other_x -= _PI;

    // Convert x into a fixed point format where where Pi is 2^24.
    const double conversion_factor = 5.340353715e6;
    double double_x = x;
    double_x += _PI * 1000.0;
    double_x += PI_BY_2;
    int64_t fixed_x = double_x * conversion_factor;
    
    // Clamp to range 0 <= x <= pi
    fixed_x &= (1 << 24) - 1;
    
    // Convert back to float
    x = fixed_x / conversion_factor;
    x -= PI_BY_2;

//     float error = x - other_x;
//     if (error > 0.1 || error < -0.1)
//         error = 0;

    float x3 = x * x * x;
    float x5 = x3 * x * x;
    float rv = 0.9996949 * x - 0.1656700 * x3 + 0.0075134 * x5;
    return rv;
}

#include <xmmintrin.h>
#pragma function(sqrt)
double sqrt(double x)
{
    return _mm_cvtss_f32(_mm_rsqrt_ps(_mm_set1_ps(x)));

//     float xhalf = 0.5f * x;
//     int i = *(int*)&x;         // evil floating point bit level hacking
//     i = 0x5f3759df - (i >> 1);  // what the fuck?
//     x = *(float*)&i;
//     x = x*(1.5f - (xhalf*x*x));
//     return x;
}

// struct _iobuf {
//     char *_ptr;
//     int   _cnt;
//     char *_base;
//     int   _flag;
//     int   _file;
//     int   _charbuf;
//     int   _bufsiz;
//     char *_tmpfname;
// };
// typedef struct _iobuf FILE;
// FILE *fopen(const char *filename, const char *mode)
// {
//     return NULL;
// }


extern "C" { unsigned short _fltused = 0; }

int __cdecl WinMainCRTStartup()
{
    return WinMain(0, 0, 0, 0);
}


#endif
