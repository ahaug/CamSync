#ifndef FCAM_N900_FLASH_H
#define FCAM_N900_FLASH_H

/** \file
 * The LED flash on the Nokia N900 */

#include "../Flash.h"
#include "../CircularBuffer.h"
#include "../Time.h"
#include <vector>

namespace FCam { 
    namespace N900 {

        /** The LED flash on the Nokia N900.
         * 
         * It is fairly weak, and can only fire for very specific
         * amounts of time (multiples of 54.6ms + 1ms). The N900 has a
         * rolling shutter sensor, so you will typically want to fire
         * it at the start of the exposure and keep it illuminated for
         * the entire exposure time plus the sensor's rolling shutter
         * time (\ref N900::Sensor::rollingShutterTime) - otherwise
         * you will see different scanlines illuminated by different
         * amounts. On the up-side, It has next to no recharge time so
         * you can fire it as often as you want, even within a single
         * exposure. */
        class Flash : public FCam::Flash {
          public:
            Flash();
            ~Flash();

            /** The flash on the N900 must fire for a multiple of 54.6
             * ms plus one millisecond (go figure) */
            int minDuration() {return 55600;}

            /** The flash on the N900 can fire for up to 492.4ms. */
            int maxDuration() {return 492400;}
        
            /** The flash on the N900 has a minimum brightness setting
             * of 2.0, using platform-specific units.
             * 
             * \todo Check that this is true - the flash seems to fire
             * with constant brightness no matter the brightness
             * setting.
             * 
             * \todo Calibrate the flash so we can switch to better
             * units like peak lumens. */
            float minBrightness() {return 2.0f;}

            /** The flash on the N900 has a maximum brightness
             * settings of 19.0, using platform-specific units. */
            float maxBrightness() {return 19.0f;}

            /* fire the flash for a given brightness and duration. The
             * brightness and duration will be clamped to the values
             * specified above. */
            void fire(float brightness, int duration);

            /** The N900's flash will begin to emit light about 3.45
             * ms after \ref N900::Flash::fire is called, plus or
             * minus about 100 microseconds. */
            int fireLatency() {return 3450;}            

            /** Return the brightness of the flash at a specific
             * time. The N900's flash has very little ramp-up and
             * ramp-down time, so this will either return the
             * requested brightness or zero. */
            float getBrightness(Time);

            /** Tag a frame with the state of the flash during that
             * frame. */
            void tagFrame(FCam::Frame);

          private:

            void setDuration(int);
            void setBrightness(float);

            struct FlashState {
                // Assume the flash state is piecewise constant. This
                // struct indicates a period of a constant brightness
                // starting at the given time.
                Time time;
                float brightness;
            };
            
            CircularBuffer<FlashState> flashHistory; 
        };
    }
}


#endif
