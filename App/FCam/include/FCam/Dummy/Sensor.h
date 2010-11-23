#ifndef FCAM_DUMMY_SENSOR_H
#define FCAM_DUMMY_SENSOR_H

/** \file
 * The dummy simulator sensor class.
 */

#include <pthread.h>

#include "../Sensor.h"
#include "Frame.h"
#include <FCam/Dummy/Platform.h>

namespace FCam { namespace Dummy {

    class Daemon;

    /** The Dummy simulator image sensor class. This works without any
     * camera hardware, providing simulated test images or images
     * loaded from DNG files through the same capture/getFrame
     * interface as the real sensors. Its simulation capabilites are
     * very limited, but sufficient for basic testing of UI or image
     * processing functionality using x86 Qt simulators or arm
     * emulators, such as the ones included in the Nokia Qt SDK. The
     * sensor simulates frame duration, exposure, and gain. */
    class Sensor : public FCam::Sensor { 
    public:
        Sensor(); ~Sensor();

        void capture(const FCam::Shot &);
        void capture(const Shot &);

        void capture(const std::vector<FCam::Shot> &);
        void capture(const std::vector<Shot> &);

        void stream(const FCam::Shot &);
        void stream(const Shot &);

        void stream(const std::vector<FCam::Shot> &);
        void stream(const std::vector<Shot> &);

        bool streaming();
        void stopStreaming();
        void start();
        void stop();

        virtual int maxExposure() const {return 10000000;} 

        virtual int minExposure() const {return 1;}       

        virtual int maxFrameTime() const {return 10000000;}

        virtual int minFrameTime() const {return 1;}      

        virtual float maxGain() const {return 1000.0f;}     

        virtual float minGain() const {return 1.0f;}      

        virtual Size minImageSize() const {return Size(1, 1);}

        virtual Size maxImageSize() const {return Size(3000, 2000);}

        virtual int maxHistogramRegions() const {return 4;}

        int rollingShutterTime(const FCam::Shot &) const;
            
        int framesPending() const;
        int shotsPending() const;

        const FCam::Platform &platform() {return Platform::instance();}

        FCam::Dummy::Frame getFrame();

    protected:
        
        FCam::Frame getBaseFrame() {return getFrame();}

    private:
        friend class Daemon;
        Daemon *daemon;

        void generateRequest();

        pthread_mutex_t requestMutex;

        // The currently streaming shot            
        std::vector<Shot> streamingShot;

        void enforceDropPolicy();

        int shotsPending_;
    };
}}

#endif
