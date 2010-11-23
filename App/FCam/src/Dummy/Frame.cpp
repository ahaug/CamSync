#include <FCam/Dummy/Frame.h>
#include <FCam/processing/Color.h>

namespace FCam { namespace Dummy {
    // _Frame vtable lives here
    _Frame::_Frame() {
    }

    void _Frame::rawToRGBColorMatrix(int kelvin, float *matrix) const {
        // Linear interpolation with inverse color temperature
        float alpha = (1./kelvin-1./3200)/(1./7000-1./3200);
        colorMatrixInterpolate(rawToRGB3200K,
                               rawToRGB7000K,
                               alpha, matrix);
    }
}}
