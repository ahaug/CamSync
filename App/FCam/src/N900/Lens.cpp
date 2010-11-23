#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <linux/videodev2.h>
#include <limits>

#include "FCam/N900/Lens.h"
#include "FCam/Frame.h"

#include "../Debug.h"
#include "V4L2Sensor.h"

namespace FCam { namespace N900 {
    
    Lens::Lens() : lensHistory(512) {

        // focus at infinity
        setFocus(0, -1);
    }
    
    Lens::~Lens() {
    }
    
    
    int Lens::ioctlGet(unsigned key) {
        struct v4l2_control ctrl;
        ctrl.id = key;
        int fd = V4L2Sensor::instance("/dev/video0")->getFD();
        if (fd < 0) {
            V4L2Sensor::instance("/dev/video0")->open();
            fd = V4L2Sensor::instance("/dev/video0")->getFD();
        }
        if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) < 0) {
            error(Event::DriverError, this, "VIDIOC_G_CTRL: %d, %d", key, errno);
            return -1;
        }
        return ctrl.value;
    }

    int Lens::ioctlSet(unsigned key, int val) {
        struct v4l2_control ctrl;
        ctrl.id = key;
        ctrl.value = val;
        int fd = V4L2Sensor::instance("/dev/video0")->getFD();
        if (fd < 0) {
            V4L2Sensor::instance("/dev/video0")->open();
            fd = V4L2Sensor::instance("/dev/video0")->getFD();
        }
        if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
            error(Event::DriverError, this, "VIDIOC_S_CTRL: %d = %d, %d", key, val, errno);
            return -1;
        }
        if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) < 0) {
            error(Event::DriverError, this, "VIDIOC_G_CTRL: %d, %d", key, errno);
            return -1;
        }
        return ctrl.value;
    }
    

    void Lens::setFocus(float f, float speed = -1) {

        // set the focus speed
        if (speed < 0) speed = maxFocusSpeed();
        if (speed < minFocusSpeed()) speed = minFocusSpeed();
        if (speed > maxFocusSpeed()) speed = maxFocusSpeed();

        // set the ramp mode to linear
        ioctlSet(V4L2_CID_FOCUS_AD5820_RAMP_MODE, 0);

        // the value we set is us per tick
        // = us per s / ticks per us
        int val = ioctlSet(V4L2_CID_FOCUS_AD5820_RAMP_TIME, 1000000.0f/diopterRateToTickRate(speed));

        float focusSpeed = tickRateToDiopterRate(1000000.0f / val);

        // set the focus
        if (f < farFocus()) f = farFocus();
        if (f > nearFocus()) f = nearFocus();

        int ticks = dioptersToTicks(f);
    
        int oldTicks = ioctlGet(V4L2_CID_FOCUS_ABSOLUTE);
        float oldDiopters = ticksToDiopters(oldTicks);
    
        int newTicks = ioctlSet(V4L2_CID_FOCUS_ABSOLUTE, ticks);

        Time start = Time::now();    

        f = ticksToDiopters(newTicks);
        float dDiopters = f - oldDiopters; 
        if (dDiopters < 0) dDiopters = -dDiopters;

        // time taken to focus = diopters to move / (diopters per second)
        Time end = start + 1000000 * dDiopters / focusSpeed;

        LensState s;
        s.time = start;
        s.position = oldDiopters;
        lensHistory.push(s);
        s.time = end;
        s.position = f;
        lensHistory.push(s);
    }

    float Lens::getFocus() const {
        return getFocus(Time::now());
    }
    
    float Lens::getFocus(Time t) const {
        if (lensHistory.size() && t > lensHistory[0].time) {
            return lensHistory[0].position;
        }

        for (size_t i = 0; i < lensHistory.size()-1; i++) {
            if (t < lensHistory[i+1].time) continue;
            if (t < lensHistory[i].time) {
                // linearly interpolate
                float alpha = float(t - lensHistory[i+1].time)/(lensHistory[i].time - lensHistory[i+1].time);
                return alpha * lensHistory[i].position + (1-alpha) * lensHistory[i+1].position;
            }
        }

        // uh oh, we ran out of history!
        error(Event::LensHistoryError, "Lens position at time %d %d is unknown", t.s(), t.us());
        return std::numeric_limits<float>::quiet_NaN(); // unknown
    }

    bool Lens::focusChanging() const {
        Time t = Time::now();
        return (lensHistory.size()) && (lensHistory[0].time > t);
    }

    float Lens::minFocusSpeed() const {
        // slowest speed is 1 tick / 3200 us
        return tickRateToDiopterRate(1/0.003200f);
    }
    
    float Lens::maxFocusSpeed() const {
        // fastest speed is 1 tick / 50 us
        return tickRateToDiopterRate(1/0.000050f);
    }


    float Lens::ticksToDiopters(int ticks) const {
        float d = 0.0315f*(ticks-227);
        // the lens hits internal blockers and doesn't actually move
        // beyond a certain range
        if (d < 0.0f)  d = 0.0f;
        if (d > 20.0f) d = 20.0f;
        return d;
    }

    int Lens::dioptersToTicks(float diopter) const {
        return (int)(diopter*31.746f + 227.5f);
    }
    
    
    float Lens::tickRateToDiopterRate(int ticks) const {
        return ticks * 0.0315f;
    }

    int Lens::diopterRateToTickRate(float diopter) const {
        return (int)(diopter*31.746f + 0.5f);
    }

  
    void Lens::tagFrame(FCam::Frame f) {
        float initialFocus = getFocus(f.exposureStartTime());
        float finalFocus = getFocus(f.exposureEndTime());

        f["lens.initialFocus"] = initialFocus;
        f["lens.finalFocus"] = finalFocus;
        f["lens.focus"] = (initialFocus + finalFocus)/2;
        f["lens.focusSpeed"] = (1000000.0f * (finalFocus - initialFocus)/
                                (f.exposureEndTime() - f.exposureStartTime()));

        float zoom = getZoom();
        f["lens.zoom"] = zoom;
        f["lens.initialZoom"] = zoom;
        f["lens.finalZoom"] = zoom;
        f["lens.zoomSpeed"] = 0;

        float aperture = getAperture();
        f["lens.aperture"] = aperture;
        f["lens.initialAperture"] = aperture;
        f["lens.finalAperture"] = aperture;
        f["lens.apertureSpeed"] = 0;

        // static properties of the N900's lens. In the future, we may
        // just add "lens.*" with a pointer to this lens to the tags,
        // and have the tag map lazily evaluate anything starting with
        // devicename.* by asking the appropriate device for more
        // details. For now there are only four more fields, so we
        // don't mind.
        f["lens.minZoom"] = minZoom();
        f["lens.maxZoom"] = maxZoom();
        f["lens.wideApertureMin"] = wideAperture(minZoom());
        f["lens.wideApertureMax"] = wideAperture(maxZoom());
    }

}}
