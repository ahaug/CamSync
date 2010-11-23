#ifndef FCAM_F2_FLASH_H
#define FCAM_F2_FLASH_H

#include "../Flash.h"
#include "../Time.h"
#include <vector>
#include <stdio.h>

/** \file
 * The F2 specific interface to the camera flash */
  
namespace FCam { namespace F2 {

    class PhidgetFlash;

    //! An F2 specific Flash Device for Phidgets triggered flash units.
    class Flash : public FCam::Flash {
    public:
    
        /*! The Flash constructor initializes a PhidgetDevice on behalf of the Flash 
         *  and registers the Phidget interface kit output that must be set true to fire
         * the flash.
         */
        Flash(int phidgetOutputIndex = 0);
        ~Flash();

        // duration in us
        int minDuration() {return 0;}
        int maxDuration() {return 0;}
    
        // brightness measured by average lumens over the duration
        float minBrightness() {return 0.0f;} // TODO: calibrate this
        float maxBrightness() {return 0.0f;}


        //! Without reverse engineering flash communication protocols,
        //! the best we can do is to simply trigger the flash at maximum
        //! brightness and unknown duration. For now, the arguments to
        //! this method are ignored.
        void fire(float brightness, int duration);

        //! The flash does not actually fire until 127ms after the call to fire(). 
        int fireLatency() {return 127*1000;} // TODO: calibrate this

        //! For flashes that feature strobe modes, this method begins a strobing sequence.
        void startStrobe();

        //! For flashes that feature strobe modes, this method ends a strobing sequence.
        void stopStrobe();

        // instantaneous brightness in lumens at some time in the past
        float getBrightness(Time) {return 0.0f;} // TODO: implement this
    
        // total photometric energy (lumen-seconds or Talbots) emitted over some duration
        float getBrightness(Time, Time) {return 0.0f;} // TODO: implement this
        
        void tagFrame(FCam::Frame); // TODO: implement this
        
        int latencyGuess;

        /*! This action marks the beginning of a strobing sequence on a strobe enabled flash unit. */
        class StrobeStartAction : public CopyableAction<StrobeStartAction> {
        public:
            //! Create a StrobeStartAction set to start strobing the given
            //! flash as soon as possible after the start of exposure.
            StrobeStartAction(Flash *f);
            //! Create a StrobeStartAction set to start strobing the given
            //! flash at time t microseconds after the start of exposure.
            StrobeStartAction(Flash *f, int t);
            //! Performs the StrobeStartAction, specifically calling
            //! startStrobe() on the associated F2::Flash.
            virtual void doAction();
        protected:
            //! An associated F2::Flash upon which the action will be performed.
            Flash *flash;
        };
        /*! This action marks the end of a strobing sequence on a strobe enabled flash unit. */
        class StrobeStopAction : public CopyableAction<StrobeStopAction> {
        public:
            //! Create a StrobeStopAction set to stop strobing the given
            //! flash as soon as possible after the start of exposure.
            StrobeStopAction(Flash *f);
            //! Create a StrobeStartAction set to stop strobing the given
            //! flash at time t microseconds after the start of exposure.
            StrobeStopAction(Flash *f, int time);
            //! Performs the StrobeStopAction, specifically calling
            //! stopStrobe() on the associated F2::Flash.
            virtual void doAction();
        protected:
            //! An associated F2::Flash upon which the action will be performed.
            Flash *flash;
        };

        
    private:
        //! A helper PhidgetDevice for handling Phidget communications.
        PhidgetFlash *phidgetFlash;
    private:
        //! The Phidget output index for this Flash device.
        int phidgetIndex;
    private:
        void setDuration(int);
        void setBrightness(float);

        struct FlashState {
            Time time;
            bool state;
        };
    };

}
}


#endif
