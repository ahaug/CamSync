#include <FCam/F2/Frame.h>
#include "Platform.h"

namespace FCam { namespace F2 {

    // Empty constructor for _F2 to get its vtable somewhere
    _Frame::_Frame() {}

    // Implementations of the static platform data
    // accessors. We just redirect all queries to the
    // F2::Platform class
    
    void _Frame::rawToRGBColorMatrix(int kelvin, float *matrix) const {
        F2::Platform::rawToRGBColorMatrix(kelvin, matrix);
    }
    
    const std::string &_Frame::manufacturer() const {;
        return F2::Platform::manufacturer;
    }
    
    const std::string &_Frame::model() const {
        return F2::Platform::model;
    }
    
    BayerPattern _Frame::bayerPattern() const {
        return F2::Platform::bayerPattern;
    }
    
    unsigned short _Frame::minRawValue() const {
        return F2::Platform::minRawValue;
    }
    
    unsigned short _Frame::maxRawValue() const {
        return F2::Platform::maxRawValue;
    }

}}
