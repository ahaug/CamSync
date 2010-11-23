#ifndef FCAM_N900_PLATFORM
#define FCAM_N900_PLATFORM

#include "FCam/Platform.h"

/** \file 
 * Static platform data about the N900 and its main image sensor. 
 */

namespace FCam { namespace N900 {

    /** Static platform data about the N900 and its main image sensor. */
    class Platform : public FCam::Platform {
    public:
        virtual void rawToRGBColorMatrix(int kelvin, float *matrix) const;
        virtual const std::string &manufacturer() const {static std::string s("Nokia"); return s;}
        virtual const std::string &model() const {static std::string s("Nokia N900"); return s;}

        /** The N900 produces raw values greater than or equal to zero */
        virtual unsigned short minRawValue() const {return 0;}

        /** The N900 produces raw values up to a maximum of 959 */
        virtual unsigned short maxRawValue() const {return 959;}

        /** The N900's bayer pattern (the top 2x2 block of pixels) is GRBG */
        virtual BayerPattern bayerPattern() const {return GRBG;}

        /** Access to the singleton instance of this class. Normally
         * you access the platform data via Frame::platform or
         * Sensor::platform. */
        static const Platform &instance();
    };
}}

#endif
