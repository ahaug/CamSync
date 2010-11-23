#ifndef FCAM_F2_SENSOR_H
#define FCAM_F2_SENSOR_H

//! \file 
//! F2::Sensor manages the Aptina MT9P031 image sensor on the F2
//! Frankencamera.  Using F2-specific Shot and Frame classes with
//! F2::Sensor exposes the additional functionality of the Aptina
//! sensor relative to the FCam base requirements.  This includes the
//! ability to control the sensor region-of-interest on a per-frame
//! basis.

#include "../Sensor.h"
#include "Frame.h"
#include "Shot.h"
#include <vector>
#include <pthread.h>

namespace FCam { namespace F2 {

    class Daemon;

    //! F2::Sensor manages the Aptina MT9P031 image sensor on the F2
    //! Frankencamera.  Using F2-specific F2::Shot and F2::Frame classes with
    //! F2::Sensor exposes the additional functionality of the Aptina
    //! sensor relative to the FCam base requirements.  This includes the
    //! ability to control the sensor region-of-interest on a per-frame
    //! basis.
    class Sensor : public FCam::Sensor {
    public:
            
        Sensor();
        ~Sensor();

        void capture(const FCam::Shot &);
        //! Overloaded capture for a F2::Shot
        void capture(const Shot &);

        void capture(const std::vector<FCam::Shot> &);
        //! Overloaded capture for a burst of F2::Shot
        void capture(const std::vector<Shot> &);

        void stream(const FCam::Shot &);
        //! Overloaded stream for a F2::Shot
        void stream(const Shot &);

        void stream(const std::vector<FCam::Shot> &);
        //! Overloaded stream for a burst of F2::Shot
        void stream(const std::vector<Shot> &);

        bool streaming();
        void stopStreaming();
        void start();
        void stop();

        /** \todo Fix below functions to be correct for Aptina instead of N900 */
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


        //! minimum image 640x480
        Size minImageSize() const; 
        //! maximum image without black calibration areas is 2592x1944
        Size maxImageSize() const; 
        //! all pixels on the array, including black pixels: 2752x2004
        static Size pixelArraySize(); 
        //! The rect describing the active (imaging) pixel array, in the 
        //! coordinate system used by F2::Shot::roiStartX/Y
        static Rect activeArrayRect();
        //! The rect describing the entire pixel array, including
        //! black pixels, in the coordinate system used by
        //! F2::Shot::roiStartX/Y
        static Rect pixelArrayRect(); 

        /** Overloaded version for F2::Shot. */
        int rollingShutterTime(const Shot&) const;

        int rollingShutterTime(const FCam::Shot&) const;

        int framesPending() const;
        int shotsPending() const;

        unsigned short minRawValue() const;
        unsigned short maxRawValue() const;
    
        BayerPattern bayerPattern() const;
            
        const std::string &manufacturer() const;
        const std::string &model() const;

        void rawToRGBColorMatrix(int kelvin, float *matrix) const;

        
        FCam::F2::Frame getFrame();
        

        void debugTiming(bool);

    protected:
        
        FCam::Frame getBaseFrame() {return getFrame();}

    private:
        // The currently streaming shot         
        std::vector<Shot> streamingShot;        

        // The daemon that manages the F2's sensor
        friend class Daemon;
        Daemon *daemon;
            
        // the Daemon calls this when it's time for new frames to be queued up
        void generateRequest();

        pthread_mutex_t requestMutex;
          
        // enforce the specified drop policy
        void enforceDropPolicy();

        // allow tagFrames to get at color matrix information
        const std::vector<int> &getColorTemps() const;
        const float *getColorMatrix(int i) const;

        // the number of outstanding shots
        int shotsPending_;
    };
        
    }
}


#endif
