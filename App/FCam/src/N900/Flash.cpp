#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "FCam/N900/Flash.h"
#include "FCam/Frame.h"

#include "../Debug.h"
#include "V4L2Sensor.h"

namespace FCam { namespace N900 {
    
    Flash::Flash() :  flashHistory(512) {
    }

    Flash::~Flash() {}
    
    void Flash::setBrightness(float b) {
        if (b < minBrightness()) b = minBrightness();
        if (b > maxBrightness()) b = maxBrightness();
        struct v4l2_control ctrl;
        ctrl.id = V4L2_CID_FLASH_INTENSITY;
        ctrl.value = b;
        int fd = V4L2Sensor::instance("/dev/video0")->getFD();
        if (fd < 0) {
            V4L2Sensor::instance("/dev/video0")->open();
            fd = V4L2Sensor::instance("/dev/video0")->getFD();
        }
        if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
            error(Event::DriverError, this, "VIDIOC_S_CTRL: %d = %d, %d", ctrl.id, ctrl.value, errno);
            return;
        }
    }

    
    void Flash::setDuration(int d) {
        if (d < minDuration()) d = minDuration();
        if (d > maxDuration()) d = maxDuration();
        struct v4l2_control ctrl;
        ctrl.id = V4L2_CID_FLASH_TIMEOUT;
        ctrl.value = d;
        int fd = V4L2Sensor::instance("/dev/video0")->getFD();
        if (fd < 0) {
            V4L2Sensor::instance("/dev/video0")->open();
            fd = V4L2Sensor::instance("/dev/video0")->getFD();
        }
        if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
            error(Event::DriverError, this, "VIDIOC_S_CTRL: %d = %d, %d", ctrl.id, ctrl.value, errno);
            return;
        }
    }

    void Flash::fire(float brightness, int duration) {
        Time fireTime = Time::now() + fireLatency();
        setBrightness(brightness);
        setDuration(duration);

        struct v4l2_control ctrl;
        ctrl.id = V4L2_CID_FLASH_STROBE;
        ctrl.value = 0;
        int fd = V4L2Sensor::instance("/dev/video0")->getFD();
        if (fd < 0) {
            V4L2Sensor::instance("/dev/video0")->open();
            fd = V4L2Sensor::instance("/dev/video0")->getFD();
        }
        if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
            error(Event::DriverError, this, "VIDIOC_S_CTRL: %d = %d, %d", ctrl.id, ctrl.value, errno);
        } else {
            FlashState f;
            f.time = fireTime;
            f.brightness = brightness;
            flashHistory.push(f);
            f.time = fireTime + duration;
            f.brightness = 0;
            flashHistory.push(f);
        }        
    }
        
    void Flash::tagFrame(FCam::Frame f) {
        Time t1 = f.exposureStartTime();
        Time t2 = f.exposureEndTime();

        // what was the state of the flash initially and finally
        float b1 = 0; 
        float b2 = 0; 

        for (size_t i = 0; i < flashHistory.size(); i++) {
            if (t1 > flashHistory[i].time) {
                b1 = flashHistory[i].brightness;
                break;
            }
        }

        for (size_t i = 0; i < flashHistory.size(); i++) {
            if (t2 > flashHistory[i].time) {
                b2 = flashHistory[i].brightness;
                break;
            }
        }

        // What was the last flash-turning-off event within this
        // exposure, and the first flash-turning-on event.
        int offTime = -1, onTime = -1;
        float brightness;
        for (size_t i = 0; i < flashHistory.size(); i++) {
            if (flashHistory[i].time < t1) break;
            if (flashHistory[i].time > t2) continue;
            if (flashHistory[i].brightness == 0 && offTime == -1) {
                offTime = flashHistory[i].time - t1;
            }                    
            if (flashHistory[i].brightness > 0) {
                brightness = flashHistory[i].brightness;
                onTime = flashHistory[i].time - t1;
            }
        }

        // If these guys are not set, and the initial brightness is zero, the flash didn't fire
        if ((offTime < 0 || onTime < 0) && (b1 == 0)) return;
        
        if (b1 > 0) {
            if (b2 == 0) {
                // it was on initially, turned off and stayed off
                f["flash.brightness"] = b1;
                f["flash.duration"] = offTime;
                f["flash.start"] = 0;
                f["flash.peak"] = offTime/2;
            } else {
                // was on at the start and the end of the frame
                f["flash.brightness"] = (b1+b2)/2;
                f["flash.duration"] = t2-t1;
                f["flash.start"] = 0;
                f["flash.peak"] = (t2-t1)/2;
            }
        } else {
            if (b2 > 0) {
                // off initially, turned on, stayed on
                int duration = (t2-t1) - onTime;
                f["flash.brightness"] = b2;
                f["flash.duration"] = duration;
                f["flash.start"] = onTime;
                f["flash.peak"] = onTime + duration/2;
            } else {
                // either didn't fire or pulsed somewhere in the middle
                if (onTime >= 0) {
                    // pulsed in the middle
                    f["flash.brightness"] = brightness;
                    f["flash.duration"] = offTime - onTime;
                    f["flash.start"] = onTime;
                    f["flash.peak"] = onTime + (offTime - onTime)/2;
                } else {
                    // didn't fire. No tags.
                }
            }
        }

        

    }

    float Flash::getBrightness(Time t) {
        for (size_t i = 0; i < flashHistory.size(); i++) {
            if (t > flashHistory[i].time) {
                return flashHistory[i].brightness;
            }
        }

        // uh oh, we ran out of history!
        error(Event::FlashHistoryError, "Flash brightness at time %d %d is unknown", t.s(), t.us());
        return std::numeric_limits<float>::quiet_NaN(); // unknown        
    }
       
}
}
