#ifndef FCAM_F2_FRAME_H
#define FCAM_F2_FRAME_H

//! \file
//! F2::Frame is a captured image with the 
//! additional parameters available on the F2 Frankencamera

#include "../Frame.h"
#include "Shot.h"

namespace FCam{ namespace F2 {

    class Sensor;
        
    /** A _Frame struct with added custom fields for the extra F2 capabilities. 
     * You should not instantiate a F2::_Frame directly, unless you're making dummy frames for testing
     * purposes.
     */
    struct _Frame: public FCam::_Frame {
        RowSkip::e rowSkip;
        ColSkip::e colSkip;
        RowBin::e rowBin;
        ColBin::e colBin;

        int roiStartX;
        int roiStartY;

        FCam::F2::Shot _shot;

        const FCam::F2::Shot &shot() const { return _shot; }
        const FCam::Shot &baseShot() const { return shot(); }

        // Derived frames should implement these to return static
        // platform data necessary for interpreting this frame.
        BayerPattern bayerPattern() const;
        unsigned short minRawValue() const;
        unsigned short maxRawValue() const;
        void rawToRGBColorMatrix(int kelvin, float *matrix) const;
        const std::string &manufacturer() const;
        const std::string &model() const;

        _Frame();
    };
    
    /** F2::Frame is data returned by an F2::Sensor as a result of a
     *  shot. Contains additional fields compared to a base
     *  FCam::Frame. This class is a reference counted pointer type
     *  to the real data, so pass it by value.
     */
    class Frame: public FCam::Frame {
    protected:
        const _Frame *get() const { return static_cast<_Frame*>(ptr.get()); }

    public: 

        /** Frames are normally acquired by sensor::getFrame(). The
         * Frame constructor can be used to construct dummy frames for
         * testing purposes. The Frame takes ownership of the _Frames
         * passed in. */
        Frame(_Frame *f=NULL): FCam::Frame(f) {}
            
        /** Number of pixel rows skipped per row read out. */
        RowSkip::e rowSkip() const { return get()->rowSkip; }

        /** Number of pixel columns skipped per column read out. */
        ColSkip::e colSkip() const { return get()->colSkip; }

        /** Number of pixel rows averaged together per row read out. */
        RowBin::e rowBin() const { return get()->rowBin; }

        /** Number of pixel columns averaged together per column read out. */
        ColBin::e colBin() const { return get()->colBin; }

        /** If roiCentered is false, defines the top-left corner of the region read out
         * 0 is the left edge of the active area. Negative values result in readout of the
         * black pixels used for black level calibration.
         */
        int roiStartX() const { return get()->roiStartX; }

        /** If roiCentered is false, defines the top-left corner of the region read out
         * 0 is the top edge of the active area. Negative values result in readout of the
         * black pixels used for black level calibration.
         */
        int roiStartY() const { return get()->roiStartY; }

        /** Overriden shot getter returning a F2::Shot instead of
         * a FCam::Shot
         */
        const FCam::F2::Shot &shot() const {
            return get()->shot();
        }
    
    };    
        
    }
}

#endif
