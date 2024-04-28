void AaPolyTestMain();
void AsteroidsMain();
void BenchmarkMain();
void ColourWhirlMain();
void FluidMain();
void FractalFernMain();
void GasMain();
void GravityMain();
void HarmonographMain();
void HelloWorldMain();
void LatticeBoltzmannMain(); 
void MandelbrotMain();
void Sierpinski3DMain();
void StretchBlitMain();
void TestMain();
void TwoWindowsMain();


#ifdef _MSC_VER
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main()
#endif
{
//     AaPolyTestMain();
//     AsteroidsMain();
//     BenchmarkMain();
//     ColourWhirlMain();
//     FluidMain();
//     FractalFernMain();
//     GasMain();
     GravityMain();
//     HarmonographMain();
//     HelloWorldMain();
//     LatticeBoltzmannMain();
//     MandelbrotMain();
//     Sierpinski3DMain();
//     StretchBlitMain();
//     TestMain();
//     TwoWindowsMain();

    return 0;
}
