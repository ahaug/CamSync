#ifndef FCAM_F2_PLATFORM_H

#include <string>
#include "FCam/Base.h"

namespace FCam { namespace F2 {
    
    class _Frame;
    class Sensor;

    class Platform {
        // the color matrices for this sensor and a method to interpolate them
        static float RawToRGBColorMatrix3200K[];
        static float RawToRGBColorMatrix7000K[];
        static void rawToRGBColorMatrix(int kelvin, float *matrix);
        
        // A manufacturer and model string
        static std::string manufacturer;
        static std::string model;
        
        // Details about the sensor
        static unsigned short minRawValue, maxRawValue;
        static BayerPattern bayerPattern;
 
        // grant the F2::_Frame and F2::Sensor access to this
        // static platform data
        friend class FCam::F2::_Frame;
        friend class FCam::F2::Sensor;

    };

}}

#endif
