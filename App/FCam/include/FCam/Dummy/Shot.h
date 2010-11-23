#ifndef FCAM_DUMMY_SHOT
#define FCAM_DUMMY_SHOT
#include "../Shot.h"

/** \file
 * Dummy::Shot collects extra parameters for capturing a Dummy::Frame
 * with a Dummy::Sensor. These parameters are the test pattern to use, and
 * optionally the source DNG file for the capture. */

namespace FCam { namespace Dummy {

    /** Test pattern selection for Dummy captures */
    enum TestPattern{
        BARS,          //< A set of vertical and horizontal bars in many colors
        CHECKERBOARD,  //< A pattern of colored squares
        FILE           //< Load a DNG file and use it for the image and metadata
    };

    /** Dummy::Shot collects parameters for simulating a frame capture with the Dummy sensor. */
    class Shot: public FCam::Shot {
    public:
        /* Test pattern to use in resulting Frame */
        TestPattern testPattern;
        /* If testPattern == FILE, the DNG file to load image data and metadata from */
        std::string srcFile;
        
        Shot();
        Shot(const FCam::Shot &);
        Shot(const Shot &);
        
        const Shot &operator=(const FCam::Shot &);
        
        const Shot &operator=(const Shot &);
        
    };
}}

#endif
