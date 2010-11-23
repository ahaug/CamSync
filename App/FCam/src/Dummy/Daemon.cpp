#include <FCam/Event.h>
#include <FCam/processing/DNG.h>

#include "Daemon.h"
#include "../Debug.h"

namespace FCam { namespace Dummy {

    Daemon::Daemon(Sensor *sensor): sensor(sensor), stop(false), running(false) {
    }

    Daemon::~Daemon() {
        stop = true;

        if (running) 
            pthread_join(simThread, NULL);

    }

    void Daemon::launchThreads() {
        int err = pthread_create(&simThread, NULL, daemon_launch_thread_, this);
        if (err) { 
            error(Event::InternalError, sensor, "Dummy::Sensor::Daemon: Can't launch simulation thread\n");
            return;
        }
    }

    void Daemon::run() {
        while (!stop) {
            if (!requestQueue.size()) {
                sensor->generateRequest();
            }
            
            if (!requestQueue.size()) {
                timespec sleepDuration;
                sleepDuration.tv_sec = 0;
                sleepDuration.tv_nsec = 100e6; // 100 ms
                dprintf(5, "Dummy::Sensor::Daemon: Empty queue, sleeping for a bit\n");
                nanosleep(&sleepDuration, NULL);
                continue;
            }
            dprintf(4, "Dummy::Sensor::Daemon: Processing new request\n");
            _Frame *f = requestQueue.pull();

            f->exposureStartTime = Time::now();
            f->exposureEndTime = f->exposureStartTime + f->shot().exposure;
            f->exposure = f->shot().exposure;
            f->gain = f->shot().gain;
            f->whiteBalance = f->shot().whiteBalance;
            f->testPattern = f->shot().testPattern;
            f->srcFile = f->shot().srcFile;

            timespec frameDuration;
            int duration = (f->shot().exposure > f->shot().frameTime ?
                            f->shot().exposure : f->shot().frameTime);
            frameDuration.tv_sec = duration / 1000000;
            frameDuration.tv_nsec = 1000 * (duration % 1000000);

            dprintf(4, "Dummy::Sensor::Daemon: Sleeping for frame duration %d us (%d s %d nsec) at %s\n", duration, frameDuration.tv_sec, frameDuration.tv_nsec,f->exposureStartTime.toString().c_str() );
            nanosleep(&frameDuration, NULL);
            dprintf(4, "Dummy::Sensor::Daemon: Done sleeping at %s\n", Time::now().toString().c_str() );
            f->frameTime = Time::now() - f->exposureStartTime;

            f->image = f->shot().image;
            if (f->image.autoAllocate()) {
                f->image = Image(f->image.size(), f->image.type());
            }
            
            switch(f->testPattern) {
            case BARS:
            case CHECKERBOARD:
                dprintf(4, "Dummy::Sensor::Daemon: Drawing test pattern\n");
                if (!f->image.discard()) {
                    for(unsigned int y=0; y < f->image.height(); y++) {
                        for (unsigned int x=0; x < f->image.width(); x++) {
                            int fX = 10000*x / (f->image.width()-1);
                            int fY = 10000*y / (f->image.height()-1);

                            unsigned short lum;
                            unsigned short rawR=0, rawG=0, rawB=0;

                            switch (f->testPattern) {
                            case BARS:
                                if (fY < 5000) {
                                    // Vertical bars
                                    if (fX < 2500) {
                                        lum = (fX / 100) * 900 / 25 + 100;
                                        rawR = ((fX / 100) % 2) * lum;
                                        rawG = ((fX / 100) % 2) * lum;
                                        rawB = ((fX / 100) % 2) * lum;
                                    } else if (fX < 5000) {
                                        lum = ((fX - 2500)/ 100) * 900/ 25 + 100;
                                        rawR = ((fX / 100) % 2) * lum;
                                        rawG = ((fX / 100) % 2) * lum / 100;
                                        rawB = ((fX / 100) % 2) * lum / 100;
                                    } else if (fX < 7500) {
                                        lum = ((fX - 5000)/ 100) * 900/ 25 + 100;
                                        rawR = ((fX / 100) % 2) * lum / 100;
                                        rawG = ((fX / 100) % 2) * lum;
                                        rawB = ((fX / 100) % 2) * lum / 100;
                                    } else {
                                        lum = ((fX - 7500)/ 100) * 900/ 25 + 100;
                                        rawR = ((fX / 100) % 2) * lum / 100;
                                        rawG = ((fX / 100) % 2) * lum / 100;
                                        rawB = ((fX / 100) % 2) * lum;
                                    }
                                } else {
                                    // Horizontal bars
                                    if (fX < 2500) {
                                        rawR = ((fY / 100) % 2) * 1000;
                                        rawG = ((fY / 100) % 2) * 1000;
                                        rawB = ((fY / 100) % 2) * 1000;
                                    } else if (fX < 5000) {
                                        rawR = ((fY / 100) % 2) * 1000;
                                        rawG = 10;
                                        rawB = 10;
                                    } else if (fX < 7500) {
                                        rawR = 10;
                                        rawG = ((fY / 100) % 2) * 1000;
                                        rawB = 10;
                                    } else {
                                        rawR = 10;
                                        rawG = 10;
                                        rawB = ((fY / 100) % 2) * 1000;
                                    }
                                }
                                break;
                            case CHECKERBOARD:
                                if (fX < 5000) {
                                    if (fY < 5000) {
                                        lum = fX * 900 / 5000 + 100;
                                        rawR =
                                            (((fX / 250) % 2) ^ 
                                             ((fY / 250) % 2)) *
                                            lum;
                                        rawG = rawR;
                                        rawB = rawR;
                                    } else {
                                        lum = fX * 900 / 5000 + 100;
                                        rawR = 
                                            (((fX / 250) % 2) ^ 
                                             ((fY / 250) % 2)) *
                                            lum;
                                        rawG = rawR/100;
                                        rawB = rawR/100;
                                    }
                                } else {
                                    if (fY < 5000) {
                                        lum = (fX-5000) * 900 / 5000 + 100;
                                        rawG = 
                                            (((fX / 250) % 2) ^ 
                                             ((fY / 250) % 2)) *
                                            lum;
                                        rawR = rawG/100;
                                        rawB = rawG/100;
                                    } else {
                                        lum = (fX-5000) * 900 / 5000 + 100;
                                        rawB = 
                                            (((fX / 250) % 2) ^
                                             ((fY / 250) % 2)) *
                                            lum;
                                        rawR = rawB/100;
                                        rawG = rawB/100;
                                    }
                                }
                                break;
                            default:
                                break;
                            }

                            rawR *= f->gain*f->exposure/10000;
                            rawG *= f->gain*f->exposure/10000;
                            rawB *= f->gain*f->exposure/10000;

                            switch (f->image.type()) {
                            case RGB24: {
                                unsigned char *px = f->image(x,y);
                                px[0] = rawR > 1000 ? 250 : rawR / 4;
                                px[1] = rawG > 1000 ? 250 : rawG / 4;
                                px[2] = rawB > 1000 ? 250 : rawB / 4;
                                break;
                            }
                            case RGB16: {
                                unsigned short *px = (unsigned short *)f->image(x,y);
                                unsigned char r =rawR > 1000 ? 250 : rawR / 4;
                                unsigned char g = rawG > 1000 ? 250 : rawG / 4;
                                unsigned char b = rawB > 1000 ? 250 : rawB / 4;
                                *px = ( (r / 8) | 
                                        ( (g / 4) << 5) |  
                                        ( (b / 8) << 11) );
                                break;
                            }
                            case UYVY: {
                                unsigned char *px = (unsigned char *)f->image(x,y);
                                unsigned char r =rawR > 1000 ? 250 : rawR / 4;
                                unsigned char g = rawG > 1000 ? 250 : rawG / 4;
                                unsigned char b = rawB > 1000 ? 250 : rawB / 4;
                                unsigned char y = 0.299 * r + 0.587 * g + 0.114 * b;
                                unsigned char u = 128 - 0.168736 *r - 0.331264 * g + 0.5 * b;
                                unsigned char v = 128 + 0.5*r - 0.418688*g - 0.081312*b;
                                px[0] = (x % 2) ? u : v;
                                px[1] = y;
                                break;
                            }
                            case YUV24: {
                                unsigned char *px = (unsigned char *)f->image(x,y);
                                unsigned char r =rawR > 1000 ? 250 : rawR / 4;
                                unsigned char g = rawG > 1000 ? 250 : rawG / 4;
                                unsigned char b = rawB > 1000 ? 250 : rawB / 4;
                                px[0] = 0.299 * r + 0.587 * g + 0.114 * b;
                                px[1] = 128 - 0.168736 *r - 0.331264 * g + 0.5 * b;
                                px[2] = 128 + 0.5*r - 0.418688*g - 0.081312*b;
                                break;
                            }
                            case RAW: {
                                unsigned short rawVal;
                                if ((x % 2 == 0 && y % 2 == 0) ||
                                    (x % 2 == 1 && y % 2 == 1) ) {
                                    rawVal = rawG;
                                } else if (x % 2 == 1 && y % 2 == 0) {
                                    rawVal = rawR;
                                } else {
                                    rawVal = rawB;
                                }
                                    
                                *(unsigned short *)f->image(x,y) = rawVal;
                                break; 
                            }
                            default:
                                break;
                            }                                
                        }  
                    }
                }
                f->_bayerPattern = sensor->platform().bayerPattern();
                f->_minRawValue = sensor->platform().minRawValue();
                f->_maxRawValue = sensor->platform().maxRawValue();
                f->_manufacturer = sensor->platform().manufacturer();
                f->_model = sensor->platform().model();
                sensor->platform().rawToRGBColorMatrix(3200, f->rawToRGB3200K);
                sensor->platform().rawToRGBColorMatrix(7000, f->rawToRGB7000K);
                f->processingDoneTime = Time::now();
                break;
            case FILE:
                if (f->image.type() != RAW) {
                    error(Event::InternalError, sensor, "Dummy::Sensor: Non-RAW image requested from a source DNG file. Not supported.");
                    f->image = Image();                        
                } else {
                    dprintf(4, "Dummy::Sensor::Daemon: Loading %s\n", f->srcFile.c_str());
                    FCam::Frame dng = loadDNG(f->srcFile);
                    if (!dng.valid()) {
                        error(Event::InternalError, sensor, "Dummy::Sensor: Unable to load file %s as a source Frame.", f->srcFile.c_str());
                    } else {
                        if (!f->image.discard()) {
                            f->image = dng.image();
                        } else {
                            f->image = Image(dng.image().size(), dng.image().type(), Image::Discard);
                        }
                        f->exposureStartTime = dng.exposureStartTime();
                        f->exposureEndTime = dng.exposureEndTime();
                        f->processingDoneTime = dng.processingDoneTime();
                        f->exposure = dng.exposure();
                        f->frameTime = dng.frameTime();
                        f->gain = dng.gain();
                        f->whiteBalance = dng.whiteBalance();
                        f->histogram = dng.histogram();
                        f->sharpness = dng.sharpness();
                        f->tags = dng.tags();
                        f->_bayerPattern = dng.platform().bayerPattern();
                        f->_minRawValue = dng.platform().minRawValue();
                        f->_maxRawValue = dng.platform().maxRawValue();
                        f->_manufacturer = dng.platform().manufacturer();
                        f->_model = dng.platform().model();
                        dng.platform().rawToRGBColorMatrix(3200, f->rawToRGB3200K);
                        dng.platform().rawToRGBColorMatrix(7000, f->rawToRGB7000K);
                    }
                }                
            }
            frameQueue.push(f);
        }
    }

    void *daemon_launch_thread_(void *arg) {
        Daemon *d = (Daemon *)arg;
        dprintf(DBG_MINOR, "Dummy::Sensor: Launching dummy simulator thread\n");
        d->running = true;
        d->run();
        d->running = false;
        pthread_exit(NULL);
        return NULL;
    }

}}
