#ifndef FCAM_N900_FRAME_H
#define FCAM_N900_FRAME_H

/** \file
 * The N900 Frame class.
 */

#include "../Frame.h"
#include "Platform.h"

namespace FCam { namespace N900 {

    struct _Frame : public FCam::_Frame {
        
        FCam::Shot _shot;
        const FCam::Shot &shot() const { return _shot; }
        const FCam::Shot &baseShot() const { return shot(); }
        
        const FCam::Platform &platform() const {return ::FCam::N900::Platform::instance();}
    };
    
    
    /** The N900 Frame class. It currently adds no features to the
        base frame class, apart from implementing the required
        virtual methods. This may change in the future - there are
        some unexploited features of the N900 sensor (notably,
        digital zoom). */        
    class Frame : public FCam::Frame {
    public:
        Frame(_Frame *f=NULL) : FCam::Frame(f) {}            
        ~Frame();
    };
}}

#endif
