#include <FCam/processing/Color.h>
#include "Platform.h"


namespace FCam { namespace F2 {
    
    // \todo Do full calibration for F2.
    // Copied from N900 for now
    void Platform::rawToRGBColorMatrix(int kelvin, float *matrix) {      
        float alpha = (1./kelvin-1./3200)/(1./7000-1./3200);
        colorMatrixInterpolate(RawToRGBColorMatrix3200K,
                               RawToRGBColorMatrix7000K,
                               alpha, matrix);
    }
    
    // These are quick and dirty numbers computed using a random
    // outdoor overcast day, and interior office lighting.  The 3x3
    // linear portion of the matrices are scaled so that the minimum
    // row sum is one (i.e. saturated in all channels maps to white).
    float Platform::RawToRGBColorMatrix3200K[] = {                
        1.6697,   -0.2693,   -0.4004,  -42.4346,
        -0.3576,    1.0615,    1.5949,  -37.1158,
        -0.2175,  -1.8751,    6.9640,  -26.6970
    };
        
    float Platform::RawToRGBColorMatrix7000K[] = {
        2.2997,   -0.4478,    0.1706,  -39.0923,
        -0.3826,    1.5906,   -0.2080,  -25.4311,
        -0.0888,   -0.7344,    2.2832,  -20.0826
    };
    
    std::string Platform::manufacturer = "Stanford University";
    std::string Platform::model = "F2 Frankencamera";
    unsigned short Platform::minRawValue = 0;
    unsigned short Platform::maxRawValue = 1024;
    BayerPattern Platform::bayerPattern = GRBG;
}}
