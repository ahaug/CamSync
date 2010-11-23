#ifndef FCAM_F2_H
#define FCAM_F2_H

/** \file
 * Including this file includes all the necessary components for the F2 implementation */

#include "FCam.h"

#include "F2/Flash.h"
#include "F2/Frame.h"
#include "F2/Lens.h"
#include "F2/Sensor.h"
#include "F2/Shot.h"
#include "F2/ShutterButton.h"

namespace FCam {
    /** The namespace for the F2 platform. 
     * This namespace declares new Sensor, Frame, and Shot classes
     * that expose all of the extra features available on the F2
     * platform, such as region-of-interest and variable skip/bin
     * settings.  To use it, include F2.h and use them
     * either with \c FCam::F2:Shot, or wholesale by \c using \c
     * namespace \c FCam::F2;*/
    namespace F2 {
    }
}
       

#endif
