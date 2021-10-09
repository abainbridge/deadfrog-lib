void AaPolyTestMain();
void BenchmarkMain();
void ColourWhirlMain();
void FluidMain();
void FractalFernMain();
void GasMain();
void HarmonographMain();
void HelloWorldMain();
void LatticeBoltzmannMain(); 
void MandelbrotMain();
void Sierpinski3DMain();
void StretchBlitMain();
void TestMain();


#ifdef _MSC_VER
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main()
#endif
{
//     AaPolyTestMain();
//     BenchmarkMain();
//     ColourWhirlMain();
//     FluidMain();
//     FractalFernMain();
//     GasMain();
//     HarmonographMain();
//     HelloWorldMain();
//     LatticeBoltzmannMain();
//     MandelbrotMain();
//     Sierpinski3DMain();
    StretchBlitMain();
//     TestMain();

    return 0;
}
