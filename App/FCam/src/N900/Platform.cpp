
#include <FCam/processing/Color.h>
#include <FCam/N900/Platform.h>


namespace FCam { namespace N900 {
    
    void Platform::rawToRGBColorMatrix(int kelvin, float *matrix) const {      

        // These are quick and dirty numbers computed using a random
        // outdoor overcast day, and interior office lighting.  The 3x3
        // linear portion of the matrices are scaled so that the minimum
        // row sum is one (i.e. saturated in all channels maps to white).
        static float RawToRGBColorMatrix3200K[] = {                
            1.6697,   -0.2693,   -0.4004,  -42.4346,
            -0.3576,    1.0615,    1.5949,  -37.1158,
            -0.2175,  -1.8751,    6.9640,  -26.6970
        };
        
        static float RawToRGBColorMatrix7000K[] = {
            2.2997,   -0.4478,    0.1706,  -39.0923,
            -0.3826,    1.5906,   -0.2080,  -25.4311,
            -0.0888,   -0.7344,    2.2832,  -20.0826
        };

        // Linear interpolation with inverse color temperature
        float alpha = (1./kelvin-1./3200)/(1./7000-1./3200);
        colorMatrixInterpolate(RawToRGBColorMatrix3200K,
                               RawToRGBColorMatrix7000K,
                               alpha, matrix);
    }

    const Platform &Platform::instance() {
        static Platform plat;
        return plat;
    }

}}
