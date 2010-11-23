#ifndef FCAM_DUMMY_PLATFORM_H
#define FCAM_DUMMY_PLATFORM_H
#include <string>

#include "FCam/Platform.h"

/** \file 
 * Static platform data about the Dummy platform and its pretend image sensor. 
 */

namespace FCam { namespace Dummy {

    /** Static platform data about the Dummy platform and its pretend image sensor. */
    class Platform : public FCam::Platform {
    public:
        virtual void rawToRGBColorMatrix(int kelvin, float *matrix) const;
        virtual const std::string &manufacturer() const {static std::string s("FCam"); return s;}
        virtual const std::string &model() const {static std::string s("FCam Dummy Platform"); return s;}

        /** The Dummy produces raw values greater than or equal to zero */
        virtual unsigned short minRawValue() const {return 0;}

        /** The Dummy produces raw values up to a maximum of 1023 */
        virtual unsigned short maxRawValue() const {return 1023;}

        /** The Dummy's bayer pattern (the top 2x2 block of pixels) is GRBG */
        virtual BayerPattern bayerPattern() const {return GRBG;}

        /** Access to the singleton instance of this class. Normally
         * you access the platform data via Frame::platform or
         * Sensor::platform. */
        static const Platform &instance();
    };
}}

#endif
