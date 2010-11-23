#include <FCam/processing/Color.h>
#include <FCam/Dummy/Platform.h>


namespace FCam { namespace Dummy {
    
    void Platform::rawToRGBColorMatrix(int kelvin, float *matrix) const {      

        // Utterly made up matrix - a bit of crosstalk and non-zero black level
        static float RawToRGBColorMatrix3200K[] = {                
            1.20, -0.1, -0.1, -10,
            -0.1, 1.20, -0.1, -10,
            -0.1, -0.1, 1.20, -10
        };
        
        // Utterly made up matrix - a bit of crosstalk and non-zero black level
        static float RawToRGBColorMatrix7000K[] = {
            1.40, -0.2, -0.2, -10,
            -0.15, 1.30, -0.15, -10,
            0.1, 0.1, 1.00, -10
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
