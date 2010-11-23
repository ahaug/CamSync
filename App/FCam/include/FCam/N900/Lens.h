#ifndef FCAM_N900_LENS_H
#define FCAM_N900_LENS_H

/** \file
 * The N900 Lens. */

#include "../Lens.h"
#include "../CircularBuffer.h"
#include "../Time.h"
#include <vector>

namespace FCam { namespace N900 {
    
    /** The Lens on the Nokia N900. The N900 has a lens capable of
        focus at a programmable speed. However, the zoom and aperture
        are fixed. */
    class Lens : public FCam::Lens {
             
    public:
        Lens();
        ~Lens();
    
        void setFocus(float, float speed);
        float getFocus() const;

        /** The N900 lens focuses to infinity (zero diopters) */
        float farFocus() const {return 0.0f;}

        /** The closest focus distance is 5cm (20 diopters) */
        float nearFocus() const {return 20.0f;}        
        bool focusChanging() const; 


        /** The N900's lens can change focus as slowly as 90 diopters
         * per second, which covers the entire focal range in about
         * 2.2s. If you want a speed slower than this, you'll have to
         * step the lens through discrete positions.
         */
        float minFocusSpeed() const;
        
        /** The N900's lens can change focus as quickly as 5800 diopters
         * per second, which covers the entire focal range in about
         * 34ms. */
        float maxFocusSpeed() const;


        /* The lens starts moving about 175 microseconds after
         * setFocus is called.
         *
         * \todo This is a guess based on the fact setFocus takes
         * about 350us. We would like to calibrate this better.
         *
         */
        int focusLatency() const {return 175;}

        /** The N900's lens is fixed at a focal length of 5.2mm */
        void setZoom(float, float) {}

        /** The N900's lens is fixed at a focal length of 5.2mm */
        float getZoom() const {return 5.2f;}

        /** The N900's lens is fixed at a focal length of 5.2mm */
        float minZoom() const {return 5.2f;}

        /** The N900's lens is fixed at a focal length of 5.2mm */
        float maxZoom() const {return 5.2f;}

        /** The N900's lens is fixed at a focal length of 5.2mm */
        bool zoomChanging() const {return false;}

        /** The N900's lens is fixed at a focal length of 5.2mm */
        float minZoomSpeed() const {return 0;}

        /** The N900's lens is fixed at a focal length of 5.2mm */
        float maxZoomSpeed() const {return 0;}

        /** The N900's lens is fixed at a focal length of 5.2mm */
        int zoomLatency() const {return 0;}
    
        /** The N900's aperture is fixed at F/2.8 */
        void setAperture(float, float) {}

        /** The N900's aperture is fixed at F/2.8 */
        float getAperture() const {return 2.8f;}

        /** The N900's aperture is fixed at F/2.8 */
        float wideAperture(float) const {return 2.8f;}

        /** The N900's aperture is fixed at F/2.8 */
        float narrowAperture(float) const {return 2.8f;}

        /** The N900's aperture is fixed at F/2.8 */
        bool apertureChanging() const {return false;}

        /** The N900's aperture is fixed at F/2.8 */
        int apertureLatency() const {return 0;}

        /** The N900's aperture is fixed at F/2.8 */
        float minApertureSpeed() const {return 0.0f;}

        /** The N900's aperture is fixed at F/2.8 */
        float maxApertureSpeed() const {return 0.0f;}

        /** Tag a frame with the state of the Lens during that
         * frame. See \ref Lens::Tags */
        void tagFrame(FCam::Frame);

        /** What was the focus at some time in the past? Uses linear
         * interpolation from known lens positions. */
        float getFocus(Time t) const;

    private:

        int ioctlSet(unsigned key, int val);
        int ioctlGet(unsigned key);
        
        float ticksToDiopters(int) const;
        int dioptersToTicks(float) const;
        float tickRateToDiopterRate(int) const;
        int diopterRateToTickRate(float) const;
        
        struct LensState {
            Time time;
            float position;
        };
        CircularBuffer<LensState> lensHistory;          
    };

}}

#endif
