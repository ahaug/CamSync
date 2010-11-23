#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <string.h>
#include <linux/videodev2.h>

#include "FCam/Time.h"
#include "FCam/Action.h"

#include "Daemon.h"
#include "../Debug.h"

// local copy of kernel headers - keep in sync!
#warning Do not forget to update the mt9p031.h header when changing the kernel!
#include "linux/mt9p031.h"

namespace FCam { namespace F2 {

    void *daemon_setter_thread_(void *arg) {
        Daemon *d = (Daemon *)arg;
        d->runSetter();    
        d->setterRunning = false;    
        pthread_exit(NULL);
    } 

    void *daemon_handler_thread_(void *arg) {
        Daemon *d = (Daemon *)arg;
        d->runHandler();
        d->handlerRunning = false;
        pthread_exit(NULL);    
    }

    void *daemon_action_thread_(void *arg) {
        Daemon *d = (Daemon *)arg;
        d->runAction();
        d->actionRunning = false;
        pthread_exit(NULL);
    }

    Daemon::Daemon(Sensor *_sensor) :
        sensor(_sensor),
        stop(false), 
        frameLimit(128),
        dropPolicy(FCam::Sensor::DropNewest),
        setterRunning(false), 
        handlerRunning(false), 
        actionRunning(false),
        waitingForFirstRequest(true),
        debugMode(false) {

        // tie ourselves to the correct sensor
        v4l2Sensor = V4L2Sensor::instance("/dev/video0");

        // launch the daemon thread with realtime priority

        // check I'm root
        if (getuid()) {
            printf("F2 camera daemon can only run as root. Aborting.\n");
            return;
        }
    
        // enable real-time scheduling modes
        FILE *f = fopen("/proc/sys/kernel/sched_rt_runtime_us", "w");
        if (f) {
            fprintf(f, "-1");
            fclose(f);
        } else {
            printf("Could not enable real-time scheduling modes, daemon thread creating may fail\n");
        }

        // make the mutexes for the producer-consumer queues
        if ((errno = 
             -(pthread_mutex_init(&actionQueueMutex, NULL) ||
               pthread_mutex_init(&cameraMutex, NULL)))) {
            perror("Error creating mutexes");
        }
    
        // make the semaphores
        sem_init(&actionQueueSemaphore, 0, 0);

        // make the semaphore for pipeline flushes
        pipelineFlush = true;
    
    }

    void Daemon::launchThreads() {

        pthread_attr_t attr;
        struct sched_param param;

        // make the setter thread

        param.sched_priority = sched_get_priority_min(SCHED_FIFO)+1;

        pthread_attr_init(&attr);

        if ((errno =
             -(pthread_attr_setschedparam(&attr, &param) ||
               pthread_attr_setschedpolicy(&attr, SCHED_FIFO) ||
               pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) ||
               pthread_create(&setterThread, &attr, daemon_setter_thread_, this)))) {
            perror("Error creating daemon setter thread");
            return;
        } else {
            setterRunning = true;
        }

        // make the handler thread
        param.sched_priority = sched_get_priority_min(SCHED_FIFO);

        if ((errno =
             -(pthread_attr_setschedparam(&attr, &param) ||
               pthread_attr_setschedpolicy(&attr, SCHED_FIFO) ||
               pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) ||
               pthread_create(&handlerThread, &attr, daemon_handler_thread_, this)))) {
            perror("Error creating daemon handler thread");
            return;
        } else {
            handlerRunning = true;
        }

        // make the actions thread
        param.sched_priority = sched_get_priority_max(SCHED_FIFO);

        if ((errno =
             -(pthread_attr_setschedparam(&attr, &param) ||
               pthread_attr_setschedpolicy(&attr, SCHED_FIFO) ||
               pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) ||
               pthread_create(&actionThread, &attr, daemon_action_thread_, this)))) {
            perror("Error creating daemon action thread");
            return;
        } else {
            actionRunning = true;
        }

        pthread_attr_destroy(&attr);
    }

    Daemon::~Daemon() {
        stop = true;

        // post a wakeup call to the action thread
        sem_post(&actionQueueSemaphore);

        if (setterRunning) 
            pthread_join(setterThread, NULL);
    
        if (handlerRunning)
            pthread_join(handlerThread, NULL);

        if (actionRunning)
            pthread_join(actionThread, NULL);

        pthread_mutex_destroy(&actionQueueMutex);
        pthread_mutex_destroy(&cameraMutex);

        v4l2Sensor->close();
    }

    void Daemon::setDropPolicy(FCam::Sensor::DropPolicy p, int f) {
        dropPolicy = p;
        frameLimit = f;
        enforceDropPolicy();
    }

    void Daemon::enforceDropPolicy() {
        if (frameQueue.size() > frameLimit) {
            printf("WARNING: frame limit hit (%d), silently dropping %d frames.\n"
                   "You're not draining the frame queue quickly enough. Use longer \n"
                   "frame times or drain the frame queue until empty every time you \n"
                   "call getFrame()\n", frameLimit, frameQueue.size() - frameLimit);
            if (dropPolicy == FCam::Sensor::DropOldest) {
                while (frameQueue.size() >= frameLimit) {
                    _Frame *f = frameQueue.pull();
                    delete f;
                }
            } else if (dropPolicy == FCam::Sensor::DropNewest) {
                while (frameQueue.size() >= frameLimit) {
                    _Frame *f = frameQueue.pullBack();
                    delete f;
                }
            } else {
                printf("Unknown drop policy! Not dropping frames.\n");
            }
        }
    
    }

    void Daemon::debugTiming(bool enable) {
        debugMode = enable;
    }

    void Daemon::runSetter() {

        // The first tick of the universe
        // Lyme disease doesn't even exist yet.
        tickSetter(Time::now());
        while (!stop) {
            //printf("Daemon ioctl hsvs wait\n");
            int err;
            timeval hs_vs_stamp = {0,0};
            v4l2_control ctrl;
            ctrl.id = MT9P031_CID_WAIT_HSVS_1;
            ctrl.value = 0; // Irrelevant for a get
        
            int currentFD = v4l2Sensor->getFD();

            Time now = Time::now();
            //printf("HSVS\n");
            /* Not using sensor->getControl() here because of 
               real possibility of timeout (error condition) */
            if (ioctl(currentFD, VIDIOC_G_CTRL, &ctrl) != 0) {
                printf("HSVS error\n");
            } else {
                if (ctrl.value == -1) {
                    printf("HSVS waiting timeout (probably)\n");
                    hs_vs_stamp.tv_sec = now.s();
                    hs_vs_stamp.tv_usec = now.us();
                } else {
                    //printf("Sec: %d\n", ctrl.value);
                    hs_vs_stamp.tv_sec = ctrl.value;
            
                    ctrl.id = MT9P031_CID_WAIT_HSVS_2;
                    err = ioctl(currentFD, VIDIOC_G_CTRL, &ctrl);
                    if (err != 0) {
                        perror ("Error reading HSVS_2");
                    } else {                
                        hs_vs_stamp.tv_usec = ctrl.value;
                    }
                }
                //printf("USec: %d\n", ctrl.value);
                //printf("Now: %d %d, DeltaT HSVS: %d usec\n", now.s(), now.us(), Time(hs_vs_stamp)-now);
                tickSetter(Time(hs_vs_stamp));          
            }
        }
    
    }

    void Daemon::setReadoutParams(_Frame *req, bool modeSwitch) {

        bool updatedParameters = false;

        // Do atomic write of all the above 
        if (updatedParameters) 
            v4l2Sensor->setControl(MT9P031_CID_WRITE_PARAMS, 0);

    }

    void Daemon::setTimes(_Frame *req, const Time &hs_vs, bool modeSwitch) {
        // how long will the previous frame take to readout
        static int lastReadout = 0; //30000;   
        static bool firstFrame = true;
        int modeSwitchLag;

        if (req) {

            if (firstFrame) {
                modeSwitchLag = req->frameTime;
                firstFrame = false;
            } else {
                modeSwitchLag = modeSwitch ? 20000 + req->frameTime : 0;   
            }

            // Predict when this frame will be done. It should be HS_VS +
            // the readout time for the previous frame + the frame time
            // for this request + however long the ISP takes to process
            // this frame.
            req->processingDoneTime = hs_vs + modeSwitchLag;
        
            // first there's the readout time for the previous frame
            req->processingDoneTime += lastReadout;
        
            // then there's the time to expose and read out this frame
            req->processingDoneTime += req->frameTime;
        
            // then there's some significant time inside the ISP if it's YUV and large
            // (this may be N900 specific)
            int ispTime = 0;
            if (req->image.type() == UYVY) {
                if (req->image.height() > 1024 && req->image.width() > 1024) {
                    ispTime = 60000;
                }
            }
            req->processingDoneTime += ispTime;            
        
            // Also compute when the exposure starts and finishes
            req->exposureStartTime = hs_vs + modeSwitchLag 
                + req->frameTime - req->exposure;

            // Now update the readout time for this frame. This formula
            // is specific to the mt9p031 sensor on the F2.
            if (req->shot().colSkip != prevShot.colSkip ||
                req->shot().colBin != prevShot.colBin ||
                req->shot().rowSkip != prevShot.rowSkip ||
                req->shot().rowBin != prevShot.rowBin) {
                if (req->shot().colSkip > prevShot.colSkip) {
                    //lastReadout = 35000;
                    //lastReadout = 15000;
                    //printf("Increasing skipping, lr: %d\n", lastReadout);
                } else {
                    //lastReadout = 35000;
                    //printf("Decreasing skipping, lr: %d\n", lastReadout);
                }
            } else {
                //lastReadout = 0; //(current.image.size.height > 1008) ? 76000 : 33000;
            }
            //lastReadout = 30000;

            req->exposureEndTime  = req->exposureStartTime + req->exposure + lastReadout;

            dprintf(2,"  Predicted time taken to read out previous frame: %d\n", lastReadout);
            dprintf(2,"  Predicted time to expose and read out this frame: %d\n", req->frameTime);
            dprintf(2,"  Predicted time to take in the ISP: %d\n", ispTime);
            dprintf(2,"  Predicted processingDone: %d %d\n", req->processingDoneTime.s(), req->processingDoneTime.us());
            dprintf(2,"  Predicted exposureEndTime: %d %d\n", req->exposureEndTime.s(), req->exposureEndTime.us());
        

        } else {
            // a bubble!
            //lastReadout = (current.image.size.height > 1008) ? 76000 : 33000;
            //lastReadout = 0;
        }
    }

    void Daemon::setExposureParams(_Frame *req, bool modeSwitch) {
        bool updatedParameters = false;

        // Set ROI parameters
        if (req->shot().rowSkip != current._shot.rowSkip || modeSwitch){
            v4l2Sensor->setControl(MT9P031_CID_ROW_SKIPPING, rowSkipDriver(req->shot().rowSkip) );
            current._shot.rowSkip = req->shot().rowSkip;
            updatedParameters = true;
        }
        if (req->shot().colSkip != current._shot.colSkip || modeSwitch){
            v4l2Sensor->setControl(MT9P031_CID_COL_SKIPPING, colSkipDriver(req->shot().colSkip) );
            current._shot.colSkip = req->shot().colSkip;
            updatedParameters = true;
        }
        if (req->shot().rowBin != current._shot.rowBin || modeSwitch){
            v4l2Sensor->setControl(MT9P031_CID_ROW_BINNING, rowBinDriver(req->shot().rowBin) );
            current._shot.rowBin = req->shot().rowBin;
            updatedParameters = true;
        }
        if (req->shot().colBin != current._shot.colBin || modeSwitch){
            v4l2Sensor->setControl(MT9P031_CID_COL_BINNING, colBinDriver(req->shot().colBin) );
            current._shot.colBin = req->shot().colBin;
            updatedParameters = true;
        }
        if (req->shot().roiCentered ) {
            if (!current._shot.roiCentered || modeSwitch) {
                v4l2Sensor->setControl(MT9P031_CID_ROI_X, 
                                   MT9P031_ROI_AUTO_X);
                v4l2Sensor->setControl(MT9P031_CID_ROI_Y, 
                                   MT9P031_ROI_AUTO_Y);
                updatedParameters = true;
            }
        } else if (current._shot.roiCentered 
                   || req->shot().roiStartX != current._shot.roiStartX 
                   || req->shot().roiStartY != current._shot.roiStartY
                   || modeSwitch) {
            v4l2Sensor->setControl(MT9P031_CID_ROI_X,
                               req->shot().roiStartX);
            v4l2Sensor->setControl(MT9P031_CID_ROI_Y,
                               req->shot().roiStartY);
            updatedParameters = true;
        }
        current._shot.roiCentered = req->shot().roiCentered;
        current._shot.roiStartX = req->shot().roiStartX;
        current._shot.roiStartY = req->shot().roiStartY;

        // Set the exposure and frame time
        if (req->shot().frameTime != current._shot.frameTime
            || modeSwitch) {
            dprintf(2,"new ft: %d\n", req->shot().frameTime);
            v4l2Sensor->setFrameTime(req->shot().frameTime);
            current._shot.frameTime = req->shot().frameTime;
            updatedParameters = true;
        }
        if (req->shot().exposure != current._shot.exposure
            || modeSwitch) {
            dprintf(2,"new exposure: %d\n", req->shot().exposure);
            v4l2Sensor->setExposure(req->shot().exposure);
            current._shot.exposure  = req->shot().exposure;
            updatedParameters = true;
        }

        // Set the gain

        if (req->shot().gain != current._shot.gain || modeSwitch) {
            v4l2Sensor->setGain(req->shot().gain);
            current._shot.gain = req->shot().gain;     
            updatedParameters = true;
        }


        // Send the atomic setting write request
        if (updatedParameters)
            v4l2Sensor->setControl(MT9P031_CID_WRITE_PARAMS, 0);
    
        // Tag the request with the actual params. Also store them in
        // current to avoid unnecessary ioctls (gets never cause I2C).
        current.gain = req->gain = v4l2Sensor->getGain();

        current.exposure  = req->exposure  = v4l2Sensor->getExposure();
        current.frameTime = req->frameTime = v4l2Sensor->getFrameTime();

        dprintf(2,"actual exposure: %d\n", req->exposure);
        dprintf(2,"actual ft: %d\n", req->frameTime);

        current.rowSkip = req->rowSkip = 
            rowSkipFCam(v4l2Sensor->getControl(MT9P031_CID_ROW_SKIPPING));
        current.colSkip = req->colSkip = 
            colSkipFCam(v4l2Sensor->getControl(MT9P031_CID_COL_SKIPPING));
        current.rowBin = req->rowBin = 
            rowBinFCam(v4l2Sensor->getControl(MT9P031_CID_ROW_BINNING));
        current.colBin = req->colBin = 
            colBinFCam(v4l2Sensor->getControl(MT9P031_CID_COL_BINNING));
        current.roiStartX = req->roiStartX =
            v4l2Sensor->getControl(MT9P031_CID_ROI_X);
        current.roiStartX = req->roiStartY =
            v4l2Sensor->getControl(MT9P031_CID_ROI_Y);


        //    printf("  tickSetter took %d us\n", after2-after);

        //printf("Rs: %d, expected ft %d\n", current.rowSkip, current.frameTime);
    }


    void Daemon::tickSetter(Time hs_vs) {
        static _Frame *req = NULL;
        static Time prev_hs_vs;
        static bool wasModeswitch = false;


        //printf("HSVS %d %d, delta %.3f ms\n", hs_vs.s(), hs_vs.us(), (hs_vs-prev_hs_vs)/1000.0);
    repeat_modeswitch:
        prev_hs_vs = hs_vs;

        ///// 
        // Step 1 - finish dealing with second half of last request (final parameters, actions)

        usleep(1000); // Get past real HSVS

        // The MT9P031 apparently needs all its parameters set at the same time. This isn't per spec,
        // but does appear to work.  
        if (req) {
            //printf("TS: Setting sensor params\n");        
        }

        // setTimes must be done after setExposureParams, or old time estimates will be used
        setTimes(req, hs_vs, wasModeswitch);
        wasModeswitch = false;

        if (req) {
            dprintf(1,"TS: Pushing to inflight\n");
            // set the sensor readout parameters and predicted done time on the pending request
            // and then push it onto the handler's input queue and the v4l2 buffer queue       
            //setReadoutParams(req);

            // now queue up this request's actions
            pthread_mutex_lock(&actionQueueMutex);
            for (std::set<FCam::Action*>::iterator i = req->shot().actions.begin();
                 i != req->shot().actions.end();
                 i++) {
                Action a;
                a.time = req->exposureStartTime + (*i)->time - (*i)->latency;
                a.action = (*i)->copy();
                actionQueue.push(a);
            }
            pthread_mutex_unlock(&actionQueueMutex);
            for (size_t i = 0; i < req->shot().actions.size(); i++) {
                sem_post(&actionQueueSemaphore);
            }

            inFlightQueue.push(req);
            req = NULL;
        } else {
            // an unpredictable bubble, so do nothing
        }

        /////
        // Step 2 - Start processing next request (exposure, frame time, mode switches)
        // If the queue is empty, try to generate some requests from a stream
        if (!requestQueue.size()) {
            dprintf(1,"TS: Asking the sensor to generate a request\n");
            sensor->generateRequest();
        }

        if (requestQueue.size()) {
            dprintf(1,"TS: Grabbing next request \n");
            // peek at the next request
            req = requestQueue.front();
        } else {
            // There are no pending requests, push a bubble into the
            // pipeline. The default parameters for a frame work nicely as
            // a bubble (as short as possible, no stats generated), except 
            // that it should be unwanted.
            dprintf(1,"TS: Creating a bubble request\n");
            _Frame *req = new _Frame;

            req->_shot.wanted = false;

            // bubbles should just run at whatever resolution is going. If
            // fast mode switches were possible, it might be nice to run
            // at the minimum resolution to go even faster, but they're
            // not.
            req->_shot.image = Image(current._shot.image.size(), current._shot.image.type(), Image::Discard);

            // Temporary hack - keep the same exposure to keep everything flowing
            // smoothly.  This is bad for long exposure bubbles!
            req->_shot.exposure = current._shot.exposure;

            // generate histograms and sharpness maps if they're going,
            // but drop the data.
            req->_shot.histogram  = current._shot.histogram;
            req->_shot.sharpness  = current._shot.sharpness;

            // push the bubble into the pipe
            requestQueue.push(req);
        }
        prevShot = current._shot; // Keep around the settings for one more go-around

        //printf("TS: Checking mode switch\n");
        // Check if the next request requires a mode switch
        if (req->shot().image.size() != current._shot.image.size() ||
            req->shot().image.type() != current._shot.image.type() ||
            req->shot().histogram  != current._shot.histogram  ||
            req->shot().sharpness  != current._shot.sharpness) {

            printf("TS:   Mode Requested %d %d\n", req->shot().image.width(), req->shot().image.height());
            printf("TS:   Mode Current %d %d\n", current._shot.image.width(), current._shot.image.height());
        
            // flush the pipeline
            dprintf(1,"Setter: Mode switch required - flushing pipe\n");
            pipelineFlush = true;

            pthread_mutex_lock(&cameraMutex);
            dprintf(1,"Setter: Handler done flushing pipe, passing control back to setter\n");
        
            // do the mode switch
        
            if (current.image.width() > 0) {
                printf("Setter: Stopping camera for mode switch\n");
                v4l2Sensor->stopStreaming();
                v4l2Sensor->close();
                v4l2Sensor->open();
            } else {
                printf("Setter: Opening camera for the first time\n");
                v4l2Sensor->open();
            }

            // Ensure we're in direct control mode (sensor doesn't auto-update parameters on frame bounds)
            v4l2Sensor->setControl(MT9P031_CID_DIRECT_MODE, 1);
        
            // Set parameters before starting stream
            setReadoutParams(req, true);
            setExposureParams(req, true);

            // set all the params for the new frame
            V4L2Sensor::Mode m;
            m.width  = req->shot().image.width();
            m.height = req->shot().image.height();
            m.type   = req->shot().image.type();
        
            v4l2Sensor->startStreaming(m, req->shot().histogram, req->shot().sharpness);

            /* Read this again because we actually told the sensor to switch modes,
               and only now does it know its new resolution */
            current.frameTime = req->frameTime = v4l2Sensor->getFrameTime();
            dprintf(1,"Setter: First frame time %f ms\n", current.frameTime/1000.);

            hs_vs = Time::now();
            dprintf(1,"Setter: Setter done bringing up camera, passing control back to handler\n");
            pipelineFlush = false;
            pthread_mutex_unlock(&cameraMutex);
        
            m = v4l2Sensor->getMode();
            req->image = Image(m.width, m.height, m.type, Image::Discard);
        
            current._shot.image = req->shot().image;
            current._shot.histogram  = req->shot().histogram;
            current._shot.sharpness  = req->shot().sharpness;

            current.image = req->image;

            dprintf(2,"TS: Popping mode switch request\n");
            // pop the request      
            requestQueue.pop();

            dprintf(1,"TS: Mode switch done at %d %d, delta %f ms\n", hs_vs.s(), hs_vs.us(), (hs_vs-prev_hs_vs)/1000.0);
            //hs_vs += req->frame_time;
            wasModeswitch = true;
            goto repeat_modeswitch;
        } 
    
        dprintf(2,"Setter: no mode switch required\n");
        Time b = Time::now();
        setReadoutParams(req);
        setExposureParams(req);
        Time a = Time::now();
        dprintf(2,"    Time to set %f ms\n", (a - b) / 1000.0);

        dprintf(2,"TS: Popping request\n");
        // pop the request 
        requestQueue.pop();

        req->image = current.image;
        //req->histogram = req->shot().histogram;
        //req->sharpness = req->shot().sharpness;

        // Exposure and frame time are set. Return and wait for the next
        // HS_VS before setting readout parameters for this request. (and pulling the next request).
        //usleep(500);

    }


    void Daemon::runHandler() {
        _Frame *req = NULL;
        V4L2Sensor::V4L2Frame *f = NULL;
        Time prevDoneTime = Time::now();

        pthread_mutex_lock(&cameraMutex);

        while (!stop) {

            // the setter may be waiting for me to finish processing
            // outstanding requests
            if (!req && pipelineFlush && inFlightQueue.size() == 0) { 
                printf("Handler: Handler done flushing pipe, passing control back to setter\n");
                while (pipelineFlush) {
                    pthread_mutex_unlock(&cameraMutex);
                    // let the setter grab the mutex. It has higher priority,
                    // so it should happen instantly. We put this in a while
                    // loop just to be sure.
                    usleep(10000);
                    pthread_mutex_lock(&cameraMutex);
                }
                printf("Handler: Setter done bringing up camera, passing control back to handler\n");
            
            }
        
            if (pipelineFlush) {
                printf("Handler: Setter would like me to flush the pipeline, but I have requests in flight\n");
            }
        
            // wait for a frame
            if (!f) {
                f = v4l2Sensor->acquireFrame(true);
            }

            if (!f) {
                printf("Handler got a NULL frame\n");
                usleep(10000);
                continue;
            } 

            dprintf(2,"Handler: frame %d %d, delta %f\n",
                    f->processingDoneTime.s(), f->processingDoneTime.us(),
                    (f->processingDoneTime-prevDoneTime)/1000.);
            prevDoneTime = f->processingDoneTime;

            // grab a request to match to it
            if (!req) {
                if (inFlightQueue.size()) {
                    //.printf("Handler: popping a frame request\n");
                    req = inFlightQueue.pull();
                } else {
                    // there's no request for this frame - probably coming up
                    // from a mode switch or starting up
                    //printf("Handler: Got a frame without an outstanding request. Waiting some for one.\n");
                    int timeout = 5;
                    while (timeout > 0 && (inFlightQueue.size()==0) ) {
                        timeout--;
                        usleep(1000);
                    }
                    if (timeout > 0) {
                        req = inFlightQueue.pull();
                    } else {
                        printf("  Giving up on waiting for request\n");
                        v4l2Sensor->releaseFrame(f);
                        f = NULL;
                        continue;
                    }
                }
            }

            dprintf(1,"Handler:     Expected at %d %d, delta %f ms\n", 
                    req->processingDoneTime.s(), req->processingDoneTime.us(),
                    (f->processingDoneTime - req->processingDoneTime)/1000.);
        
            // at this point we have a frame and a request, now look at
            // the time delta between them to see if they're a match
            int dt = req->processingDoneTime - f->processingDoneTime;

            if (dt < -25000 && !debugMode) { // more than 25 ms late
                dprintf(0,"Handler: Expected a frame at %d %d, but one didn't arrive until %d %d, dt %f ms\n",
                       req->processingDoneTime.s(), req->processingDoneTime.us(),
                       f->processingDoneTime.s(), f->processingDoneTime.us(),
                       dt/1000.0);
                req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                if (!req->shot().wanted) {
                    /* Get rid of bubble allocated by setter */
                    delete req;
                } else {
                    // the histogram and sharpness map may still have appeared
                    req->histogram = v4l2Sensor->getHistogram(req->exposureEndTime-2300, 
                                                              req->shot().histogram);
                    req->sharpness = v4l2Sensor->getSharpnessMap(req->exposureEndTime-2300,
                                                                 req->shot().sharpness);

                    frameQueue.push(req);
                    enforceDropPolicy();
                }
                req = NULL;
            } else if (dt < 15000 || debugMode) { // In debugMode, accept all comers
                // Is this frame wanted or a bubble?
                if (!req->shot().wanted) {
                    // it's a bubble - drop it
                    //printf("Handler: discarding a bubble\n");
                    delete req;
                    v4l2Sensor->releaseFrame(f);
                } else {
                
                    // this looks like a match - bag and tag it
                    req->processingDoneTime = f->processingDoneTime;

                    size_t bytes = req->image.width()*req->image.height()*2;
                    if (f->length < bytes) bytes = f->length;
                
                    if (req->shot().image.autoAllocate()) {
                        req->image = Image(req->image.size(), req->image.type(),f->data).copy();
                        dprintf(2,"Autoallocate: %d x %d, %d\n", 
                                req->image.width(), req->image.height(), req->image(0,0));
                    } else if (req->shot().image.discard()) {
                        req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                    } else {
                        if (req->image.size() != req->shot().image.size()) {
                            dprintf(0,"ERROR: Requested image size (%d x %d) "
                                    "on an already allocated image does not "
                                    "match actual image size (%d x %d). Dropping image data.\n",
                                   req->shot().image.width(), req->shot().image.height(),
                                   req->image.width(), req->image.height());
                            req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                        } else { // the size matches
                            req->image = req->shot().image;
                            // Need to decide on a more dynamic timeout for lock() here,
                            // but 10ms shouldn't be too bad
                            if(req->image.lock(10000)) {
                                req->image.copyFrom(Image(req->image.size(), req->image.type(),f->data));
                                req->image.unlock();
                            }  else {
                                dprintf(0,"WARNING: Daemon discarding image data (Target is locked)\n");
                                req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                            }
                        }
                    }

                    v4l2Sensor->releaseFrame(f);
                    req->histogram = v4l2Sensor->getHistogram(req->exposureEndTime-2300, 
                                                              req->shot().histogram);
                    req->sharpness = v4l2Sensor->getSharpnessMap(req->exposureEndTime-2300, 
                                                                 req->shot().sharpness);

                    frameQueue.push(req);
                    enforceDropPolicy();

                
                    //              printf("Handler: finalized a request for a %d x %d frame\n", req->image.size.width, req->image.size.height);            
                }
            
                req = NULL;
                f = NULL;

            } else { // more than 10ms early... something weird's going on
                dprintf(0,"Handler: Received an early mystery frame (%d %d) vs (%d %d), dropping it.\n",
                       f->processingDoneTime.s(), f->processingDoneTime.us(),
                       req->processingDoneTime.s(), req->processingDoneTime.us());
                v4l2Sensor->releaseFrame(f);
                f = NULL;
            }
        
        }
        pthread_mutex_unlock(&cameraMutex);
    
    }


    void Daemon::runAction() {
        printf("Action thread running...\n");
        while (1) {       
            sem_wait(&actionQueueSemaphore);
            if (stop) break;
            // priority inversion :(
            pthread_mutex_lock(&actionQueueMutex);
            Action a = actionQueue.top();
            actionQueue.pop();
            pthread_mutex_unlock(&actionQueueMutex);

            Time t = Time::now();
            int delay = (a.time - t) - 500;
            if (delay > 0) usleep(delay);
            Time before = Time::now();
            // busy wait until go time
            while (a.time > before) before = Time::now();
            a.action->doAction();
            //Time after = Time::now();
            printf("Action thread: Initiated action %d us after scheduled time\n", before - a.time);
            delete a.action;
        }
    }

} }
