#ifndef FCAM_V4L2_SENSOR_H
#define FCAM_V4L2_SENSOR_H

#include <vector>
#include <string>
#include <map>

#include "FCam/Base.h"
#include "FCam/Time.h"
#include <FCam/Histogram.h>
#include <FCam/SharpnessMap.h>

namespace FCam { namespace F2 {

    // This class gives low-level control over the sensor using the
    // V4L2 interface. It is used by the user-visible sensor object to
    // control the sensor

    class V4L2Sensor {
      public:

        struct V4L2Frame {
            Time processingDoneTime;
            unsigned char *data;
            size_t length; // in bytes
            int index;    
        };

        struct Mode {
            int width, height;
            ImageFormat type;
        };

        static V4L2Sensor *instance(std::string);

        // open and close the device
        void open();
        void close();

        // start and stop streaming
        void startStreaming(Mode, 
                            const HistogramConfig &,
                            const SharpnessMapConfig &);
        void stopStreaming();

        V4L2Frame *acquireFrame(bool blocking);
        void releaseFrame(V4L2Frame *frame);

        Mode getMode() {return currentMode;}

        enum Errors {
            InvalidId,
            OutOfRangeValue,
            ControlBusy
        };

        void setControl(unsigned int id, int value);
        int getControl(unsigned int id);

        void setExposure(int);
        int getExposure();

        void setFrameTime(int);
        int getFrameTime();

        void setGain(float);
        float getGain();

        Histogram getHistogram(Time t, const HistogramConfig &conf);
        void setHistogramConfig(const HistogramConfig &);

        SharpnessMap getSharpnessMap(Time t, const SharpnessMapConfig &conf);
        void setSharpnessMapConfig(const SharpnessMapConfig &);

        int getFD();
       
      private:
        V4L2Sensor(std::string);

        Mode currentMode;
        SharpnessMapConfig currentSharpness;
        HistogramConfig currentHistogram;

        std::vector<V4L2Frame> buffers;        

        enum {CLOSED=0, IDLE, STREAMING} state;
        int fd;

        std::string filename;

        // there is one of these objects per device
        static std::map<std::string, V4L2Sensor *> instances_;
    };

}}

#endif

