#ifndef FCAM_N900_SENSOR_H
#define FCAM_N900_SENSOR_H

/** \file 
 * The N900 Image Sensor class 
 */



#include "../Sensor.h"
#include <vector>
#include <pthread.h>
#include "Frame.h"
#include "Platform.h"

namespace FCam { namespace N900 {

    class Daemon;

    /** The N900 Image Sensor class. It takes vanilla shots and
     * returns vanilla frames. See the base class documentation
     * for the semantics of its methods. 
     * 
     * The image sensor on the N900 is natively 2592x1968, and so this
     * is the best RAW resolution to stream. N900 can also capture
     * 648x492 in RAW by "binning" (averaging down) by a factor of
     * four, and 1296x984 by binning with a factor of two. These modes
     * are not recommended, because they slightly break the assumptions
     * underlying most demosaicing algorithms.
     * 
     * In UYVY mode, the N900 can stream at a wide variety of
     * resolutions. This is done by streaming at a compatible raw
     * resolution and then resizing the resulting images in
     * hardware. All resolutions incur some degree of resampling. If
     * this bothers you, stream RAW and do the demosaicing using 
     * \ref demosaic. The hardware resizer in the OMAP3430 is quite good
     * however. Recommended resolutions to use in UYVY mode include
     * 640x480, 1280x960, 2560x1920, and 2576x1944 (which is not quite
     * 4:3).
     * 
     */
    class Sensor : public FCam::Sensor {
    public:
            
        Sensor();
        ~Sensor();
        
        void capture(const FCam::Shot &);
        void capture(const std::vector<FCam::Shot> &);
        void stream(const FCam::Shot &s);
        void stream(const std::vector<FCam::Shot> &);
        bool streaming();
        void stopStreaming();
        void start();
        void stop();

        /** The maximum exposure time on the N900 is 1080842 us (just
         * over a second) for small resolutions (height <= 960), and
         * 2489140 us for larger resolutions (about 2.5 seconds). */
        virtual int maxExposure() const {return 2489140;} 

        /** The minimum exposure time on the N900 is 38 us for small
         * resolutions (height <= 960) and 66 us for larger
         * resolutions. */
        virtual int minExposure() const {return 38;}       

        /** The maximum frame time on the N900 is 1081250 us for small
         * resolutions (just over a second), and 2490072 us for larger
         * resolutions (about 2.5 seconds). */
        virtual int maxFrameTime() const {return 2490072;}

        /** The minimum frame time on the N900 is the 33414 us for
         * smaller resolutions (height <= 960), and 77412 us for larger
         * resolutions. */
        virtual int minFrameTime() const {return 33414;}      

        /** The maximum gain on the N900 is 32, which can be considered ISO 3200. */
        virtual float maxGain() const {return 32.0f;}     

        /** The minimum supported gain is 1, which can be considered ISO 100. */
        virtual float minGain() const {return 1.0f;}      

        /** The smallest image size on the N900 is 160x120 */
        virtual Size minImageSize() const {return Size(160, 120);}

        /** The largest image size on the N900 is 2592x1968 if
         * operated in RAW mode. In UYVY, the largest image size you
         * should request is 2560x1920 */
        virtual Size maxImageSize() const {return Size(2592, 1968);}

        /** The maximum supported number of histogram regions on the N900 is 4. */
        virtual int maxHistogramRegions() const {return 4;}


        /** The N900 has a rolling shutter time of around 33ms when
         * operating at resolutions with height <= 960, and
         * 77ms for resolutions above that size. */
        int rollingShutterTime(const Shot &) const;
            
        int framesPending() const;
        int shotsPending() const;

        virtual const Platform &platform() {return N900::Platform::instance();}

        FCam::N900::Frame getFrame();

    protected:
        
        FCam::Frame getBaseFrame() {return getFrame();}

    private:
        // The currently streaming shot            
        std::vector<Shot> streamingShot;            

        // The daemon that manages the N900's sensor
        friend class Daemon;
        Daemon *daemon;
            
        // the Daemon calls this when it's time for new frames to be queued up
        void generateRequest();

        pthread_mutex_t requestMutex;
          
        // enforce the specified drop policy
        void enforceDropPolicy();

        // The number of outstanding shots
        int shotsPending_;  

        // This is so the daemon can inform the sensor that a frame
        // was dropped due to the frame limit being hit in a
        // thread-safe way
        void decShotsPending();
    };
        
}
}


#endif
