#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>

#include "FCam/Time.h"
#include "FCam/Frame.h"
#include "FCam/Action.h"

#include "../Debug.h"
#include "Daemon.h"
#include "linux/omap34xxcam-fcam.h"

namespace FCam { namespace N900 {

    void *daemon_setter_thread_(void *arg) {
        Daemon *d = (Daemon *)arg;
        d->runSetter();    
        d->setterRunning = false;    
        close(d->daemon_fd);
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

    Daemon::Daemon(Sensor *sensor) :
        sensor(sensor),
        stop(false), 
        frameLimit(128),
        dropPolicy(Sensor::DropNewest),
        setterRunning(false), 
        handlerRunning(false), 
        actionRunning(false),
        threadsLaunched(false) {
        
        // tie ourselves to the correct sensor
        v4l2Sensor = V4L2Sensor::instance("/dev/video0");

        // make the mutexes for the producer-consumer queues
        if ((errno = 
             -(pthread_mutex_init(&actionQueueMutex, NULL) ||               
               pthread_mutex_init(&cameraMutex, NULL)))) {
            error(Event::InternalError, sensor, "Error creating mutexes: %d", errno);
        }
    
        // make the semaphore
        sem_init(&actionQueueSemaphore, 0, 0);

        lastGoodShot.wanted = false;

        pipelineFlush = true;
    }

    void Daemon::launchThreads() {    
        if (threadsLaunched) return;
        threadsLaunched = true;

        // launch the threads
        pthread_attr_t attr;
        struct sched_param param;

        // Open the device as a daemon
        daemon_fd = open("/dev/video0", O_RDWR, 0);

        if (daemon_fd < 0) {
            error(Event::InternalError, sensor, "Error opening /dev/video0: %d", errno);
            return;
        }

        // Try to register myself as the fcam camera client
        if (ioctl(daemon_fd, VIDIOC_FCAM_INSTALL, NULL)) {
            if (errno == EBUSY) {
                error(Event::DriverLockedError, sensor,
                      "An FCam program is already running");
            } else {
                error(Event::DriverMissingError, sensor, 
                      "Error %d calling VIDIOC_FCAM_INSTALL: Are the FCam drivers installed?", errno);
            }
            return;
        }
        // I should now have CAP_SYS_NICE

        // make the setter thread
        param.sched_priority = sched_get_priority_min(SCHED_FIFO)+1;

        pthread_attr_init(&attr);

        if ((errno =
             -(pthread_attr_setschedparam(&attr, &param) ||
               pthread_attr_setschedpolicy(&attr, SCHED_FIFO) ||
               pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) ||
               pthread_create(&setterThread, &attr, daemon_setter_thread_, this)))) {
            error(Event::InternalError, sensor, "Error creating daemon setter thread: %d", errno);
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
            error(Event::InternalError, sensor, "Error creating daemon handler thread: %d", errno);
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
            error(Event::InternalError, sensor, "Error creating daemon action thread: %d", errno);
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

        // Clean up all the internal queues
        while (inFlightQueue.size()) delete inFlightQueue.pull();        
        while (requestQueue.size()) delete requestQueue.pull();
        while (frameQueue.size()) delete frameQueue.pull();
        while (actionQueue.size()) {
            delete actionQueue.top().action;
            actionQueue.pop();
        }

        v4l2Sensor->stopStreaming();

        v4l2Sensor->close();
    }


    void Daemon::setDropPolicy(Sensor::DropPolicy p, int f) {
        dropPolicy = p;
        frameLimit = f;
        enforceDropPolicy();
    }

    void Daemon::enforceDropPolicy() {
        if (frameQueue.size() > frameLimit) {
            warning(Event::FrameLimitHit, sensor,
                    "WARNING: frame limit hit (%d), silently dropping %d frames.\n"
                   "You're not draining the frame queue quickly enough. Use longer \n"
                   "frame times or drain the frame queue until empty every time you \n"
                   "call getFrame()\n", frameLimit, frameQueue.size() - frameLimit);
            if (dropPolicy == Sensor::DropOldest) {
                while (frameQueue.size() >= frameLimit) {
                    sensor->decShotsPending();
                    delete frameQueue.pull();
                }
            } else if (dropPolicy == Sensor::DropNewest) {
                while (frameQueue.size() >= frameLimit) {
                    sensor->decShotsPending();
                    delete frameQueue.pullBack();
                }
            } else {
                error(Event::InternalError, sensor, 
                      "Unknown drop policy! Not dropping frames.\n");
            }
        }
    }

    void Daemon::runSetter() {
        dprintf(2, "Running setter...\n"); fflush(stdout);
        tickSetter(Time::now());
        while (!stop) {
            struct timeval t;
            if (ioctl(daemon_fd, VIDIOC_FCAM_WAIT_FOR_HS_VS, &t)) {                
                if (stop) break;
                error(Event::DriverError, sensor, 
                      "error in VIDIOC_FCAM_WAIT_FOR_HS_VS: %s", strerror(errno));
                usleep(100000);
                continue;
            }
            tickSetter(Time(t));
        }
	// When shutting down, make sure we leave V4L2 with the last requested Shot's 
	// settings, so that they can be used by later programs. Otherwise, the bubbles
	// produced during shutdown will clobber them. Gain and WB are not written by bubbles
	// so don't need to be dealt with here.
        if (lastGoodShot.wanted == true) {
            v4l2Sensor->setFrameTime(lastGoodShot.frameTime);
            v4l2Sensor->setExposure(lastGoodShot.exposure);
            v4l2Sensor->setGain(lastGoodShot.gain);
        }
    }

    void Daemon::tickSetter(Time hs_vs) {
        static _Frame *req = NULL;

        dprintf(3, "Current hs_vs was at %d %d\n", hs_vs.s(), hs_vs.us());

        // how long will the previous frame take to readout
        static int lastReadout = 33000;

        static bool ignoreNextHSVS = false;

        dprintf(4, "Setter: got HS_VS\n");

        if (ignoreNextHSVS) {
            dprintf(4, "Setter: ignoring it\n");
            ignoreNextHSVS = false;
            return;
        }

        // Is there a request for which I have set resolution and exposure, but not gain and WB?
        if (req) {
            dprintf(4, "Setter: setting gain and WB\n");
            // set the gain and predicted done time on the pending request
            // and then push it onto the handler's input queue and the v4l2 buffer queue
	    // Only update gain/wb for wanted requests, bubbles shouldn't change these.
	    if (req->shot().wanted == true) {
		if (req->shot().gain != current._shot.gain) {
		    v4l2Sensor->setGain(req->shot().gain);
		    current._shot.gain = req->shot().gain;
		}
		current.gain = req->gain = v4l2Sensor->getGain();
		
		if (req->shot().colorMatrix().size() != 12) {
		    if (req->shot().whiteBalance != current._shot.whiteBalance) {                
			int wb = req->shot().whiteBalance;
			
			// Very lenient sanity checks - restricting the range is up to the auto-wb algorithm
			if (wb < 0) wb = 0; // a black-body radiator at absolute zero.
			if (wb > 25000) wb = 25000; // A type 'O' star.
			
			float matrix[12];
			sensor->platform().rawToRGBColorMatrix(wb, matrix);
			v4l2Sensor->setWhiteBalance(matrix);
			
			current._shot.whiteBalance = req->shot().whiteBalance;
			current.whiteBalance = wb;            
		    }
		    req->whiteBalance = current.whiteBalance;
		} else {
		    // custom WB - always set it                
		    v4l2Sensor->setWhiteBalance(&req->shot().colorMatrix()[0]);
		    current._shot.setColorMatrix(&req->shot().colorMatrix()[0]);
		}
	    }

            // Predict when this frame will be done. It should be HS_VS +
            // the readout time for the previous frame + the frame time
            // for this request + however long the ISP takes to process
            // this frame.
            req->processingDoneTime = hs_vs;
        
            // first there's the readout time for the previous frame
            req->processingDoneTime += lastReadout;
        
            // then there's the time to expose and read out this frame
            req->processingDoneTime += req->frameTime;

            // then there's some significant time inside the ISP if it's YUV and large
            // (this may be N900 specific)
            int ispTime = 0;
            if (req->image.type() == UYVY) {
                if (req->image.height() > 1024 && req->image.width() > 1024) {
                    ispTime = 65000;
                } else {
                    ispTime = 10000;
                }
            }
            req->processingDoneTime += ispTime;               
        
            // Also compute when the exposure starts and finishes
            req->exposureStartTime = hs_vs + req->frameTime - req->exposure;

            // Now updated the readout time for this frame. This formula
            // is specific to the toshiba et8ek8 sensor on the n900
            lastReadout = (current.image.height() > 1008) ? 76000 : 33000;

            req->exposureEndTime  = req->exposureStartTime + req->exposure + lastReadout;

            // now queue up this request's actions
            pthread_mutex_lock(&actionQueueMutex);
            for (std::set<FCam::Action*>::const_iterator i = req->shot().actions().begin();
                 i != req->shot().actions().end();
                 i++) {
                Action a;
                a.time = req->exposureStartTime + (*i)->time - (*i)->latency;
                a.action = (*i)->copy();
                actionQueue.push(a);
            }
            pthread_mutex_unlock(&actionQueueMutex);
            for (size_t i = 0; i < req->shot().actions().size(); i++) {
                sem_post(&actionQueueSemaphore);
            }

            // The setter is done with this frame. Push it into the
            // in-flight queue for the handler to deal with.
            inFlightQueue.push(req);
            req = NULL;
        } else {
            // a bubble!
            lastReadout = (current.image.height() > 1008) ? 76000 : 33000;
        }

        // grab a new request and set an appropriate exposure and
        // frame time for it make sure there's a request ready for us
        if (!requestQueue.size()) {
            sensor->generateRequest();
        }

        // Peek ahead into the request queue to see what request we're
        // going to be handling next
        if (requestQueue.size()) {
            dprintf(4, "Setter: grabbing next request\n");
            // There's a real request for us to handle
            req = requestQueue.front();
            // Keep track of the last good shot parameters so we can set those on exit
            // since bubbles will clobber the V4L2 state on shutdown otherwise.
            // This allows us to be nice to other programs hoping to use our last settings.
            if (req->_shot.wanted == true) {
                lastGoodShot.wanted = true;
                lastGoodShot.exposure = req->_shot.exposure;
                lastGoodShot.gain = req->_shot.gain;
                lastGoodShot.frameTime = req->_shot.frameTime;
            }
        } else {
            dprintf(4, "Setter: inserting a bubble\n");
            // There are no pending requests, push a bubble into the
            // pipeline. The default parameters for a frame work nicely as
            // a bubble (as short as possible, no stats generated, output
            // unwanted).
            req = new _Frame;
            req->_shot.wanted = false;

            // bubbles should just run at whatever resolution is going. If
            // fast mode switches were possible, it might be nice to run
            // at the minimum resolution to go even faster, but they're
            // not.
            req->_shot.image = Image(current._shot.image.size(), current._shot.image.type(), Image::Discard);

            // generate histograms and sharpness maps if they're going,
            // but drop the data.
            req->_shot.histogram  = current._shot.histogram;
            req->_shot.sharpness  = current._shot.sharpness;

            // push the bubble into the pipe
            requestQueue.pushFront(req);
        }

        // Check if the next request requires a mode switch
        if (req->shot().image.size() != current._shot.image.size() ||
            req->shot().image.type() != current._shot.image.type() ||
            req->shot().histogram  != current._shot.histogram  ||
            req->shot().sharpness  != current._shot.sharpness) {

            // flush the pipeline
            dprintf(3, "Setter: Mode switch required - flushing pipe\n");
            pipelineFlush = true;

            pthread_mutex_lock(&cameraMutex);
            dprintf(3, "Setter: Handler done flushing pipe, passing control back to setter\n");
        
            // do the mode switch
        
            if (current.image.width() > 0) {
                dprintf(3, "Setter: Shutting down camera\n");
                v4l2Sensor->stopStreaming();
                v4l2Sensor->close();
            }
            dprintf(3, "Setter: Starting up camera in new mode\n");
            v4l2Sensor->open();
        
            // set all the params for the new frame
            V4L2Sensor::Mode m;
            m.width  = req->shot().image.width();
            m.height = req->shot().image.height();
            // enforce UYVY or RAW
            m.type   = req->shot().image.type() == RAW ? RAW : UYVY;
            v4l2Sensor->startStreaming(m, req->shot().histogram, req->shot().sharpness);
            v4l2Sensor->setFrameTime(0);
        
            dprintf(3, "Setter: Setter done bringing up camera, passing control back "
                    "to handler. Expect two mystery frames.\n");
            pipelineFlush = false;
            pthread_mutex_unlock(&cameraMutex);
        
            m = v4l2Sensor->getMode();
            // Set destination image to new mode settings
            req->image = Image(m.width, m.height, m.type, Image::Discard);
        
            current._shot.image = Image(req->shot().image.size(), req->shot().image.type(), Image::Discard);
            current._shot.histogram  = req->shot().histogram;
            current._shot.sharpness  = req->shot().sharpness;
            
            // make sure we set everything else for the next frame,
            // because restarting streaming will have nuked it
            current._shot.frameTime = -1;
            current._shot.exposure = -1;
            current._shot.gain = -1.0;
            current._shot.whiteBalance = -1;
            current.image = Image(m.width, m.height, m.type, Image::Discard);
         
            req = NULL;
        
            // Wait for the second HS_VS before proceeding
            ignoreNextHSVS = true;
        
            return;
        } else {
            // no mode switch required
        }

        // pop the request 
        requestQueue.pop(); 

        Time next = hs_vs + current.frameTime;
        dprintf(3, "The current %d x %d frame has a frametime of %d\n", 
                current.image.width(), current.image.height(), current.frameTime);
        dprintf(3, "Predicting that the next HS_VS will be at %d %d\n",
                next.s(), next.us());
        
        int exposure = req->shot().exposure;
        int frameTime = req->shot().frameTime; 

        if (frameTime < exposure + 400) {
            frameTime = exposure + 400;
        }

        dprintf(4, "Setter: setting frametime and exposure\n");
        // Set the exposure
        v4l2Sensor->setFrameTime(frameTime);
        v4l2Sensor->setExposure(exposure);
    
        // Tag the request with the actual params. Also store them in
        // current to avoid unnecessary I2C.
        current._shot.frameTime = frameTime;
        current._shot.exposure  = exposure;
        current.exposure  = req->exposure  = v4l2Sensor->getExposure();
        current.frameTime = req->frameTime = v4l2Sensor->getFrameTime();
        req->image = current.image;

        // Exposure and frame time are set. Return and wait for the next
        // HS_VS before setting gain for this request-> (and pulling the next request).
    
        dprintf(4, "Setter: Done with this HS_VS, waiting for the next one\n");

    }


    void Daemon::runHandler() {
        _Frame *req = NULL;       
        V4L2Sensor::V4L2Frame *f = NULL;

        pthread_mutex_lock(&cameraMutex);

        while (!stop) {

            // the setter may be waiting for me to finish processing
            // outstanding requests
            if (!req && pipelineFlush && inFlightQueue.size() == 0) {        
                dprintf(3, "Handler: Handler done flushing pipe, passing control back to setter\n");
                while (pipelineFlush) {
                    pthread_mutex_unlock(&cameraMutex);
                    // let the setter grab the mutex. It has higher priority,
                    // so it should happen instantly. We put this in a while
                    // loop just to be sure.
                    usleep(10000);
                    pthread_mutex_lock(&cameraMutex);
                }
                dprintf(3, "Handler: Setter done bringing up camera, passing control back to handler\n");
            
            }
        
            if (pipelineFlush) {
                dprintf(3, "Handler: Setter would like me to flush the pipeline, but I have requests in flight\n");
            }
        
            // wait for a frame
            if (!f)
                f = v4l2Sensor->acquireFrame(true);

            if (!f) {                
                error(Event::InternalError, "Handler got a NULL frame\n");
                usleep(300000);
                continue;
            } 
        
            // grab a request to match to it
            if (!req) {
                if (inFlightQueue.size()) {
                    dprintf(4, "Handler: popping a frame request\n");
                    req = inFlightQueue.pull();
                } else {
                    // there's no request for this frame - probably coming up
                    // from a mode switch or starting up
                    dprintf(3, "Handler: Got a frame without an outstanding request,"
                            " dropping it.\n");
                    v4l2Sensor->releaseFrame(f);
                    f = NULL;
                    continue;
                }
            }
        
            // at this point we have a frame and a request, now look at
            // the time delta between them to see if they're a match
            int dt = req->processingDoneTime - f->processingDoneTime;
        
            dprintf(4, "Handler dt = %d\n", dt);
        
            if (dt < -25000) { // more than 25 ms late
                dprintf(3, "Handler: Expected a frame at %d %d, but one didn't arrive until %d %d\n",
                        req->processingDoneTime.s(), req->processingDoneTime.us(),
                        f->processingDoneTime.s(), f->processingDoneTime.us());
                error(Event::ImageDroppedError, sensor,
                      "Expected image data not returned from V4L2. Likely caused by excess"
                      " page faults (thrashing).");
                req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                if (!req->shot().wanted) {
                    delete req;
                } else {
                    // the histogram and sharpness map may still have appeared
                    req->histogram = v4l2Sensor->getHistogram(req->exposureEndTime, req->shot().histogram);
                    req->sharpness = v4l2Sensor->getSharpnessMap(req->exposureEndTime, req->shot().sharpness);
                    frameQueue.push(req);
                    enforceDropPolicy();
                }
                req = NULL;
            } else if (dt < 10000) {
                // Is this frame wanted or a bubble?
                if (!req->shot().wanted) {
                    // it's a bubble - drop it
                    dprintf(4, "Handler: discarding a bubble\n");
                    delete req;
                    v4l2Sensor->releaseFrame(f);
                } else {
                
                    // this looks like a match - bag and tag it
                    req->processingDoneTime = f->processingDoneTime;

                    size_t bytes = req->image.width()*req->image.height()*2;
                    if (f->length < bytes) bytes = f->length;

                    if (req->shot().image.autoAllocate()) {
                        req->image = Image(req->image.size(), req->image.type(), f->data).copy();
                    } else if (req->shot().image.discard()) {
                        req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                    } else {
                        if (req->image.size() != req->shot().image.size()) {
                            error(Event::ResolutionMismatch, sensor, 
                                  "Requested image size (%d x %d) "
                                  "on an already allocated image does not "
                                  "match actual image size (%d x %d). Dropping image data.",
                                  req->shot().image.width(), req->shot().image.height(),
                                  req->image.width(), req->image.height());
                            req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                            // TODO: crop instead?
                        } else if (req->image.type() != req->shot().image.type()) {
                            error(Event::FormatMismatch, sensor, 
                                  "Requested unsupported image format %d "
                                  "for an already allocated image.",
                                  req->shot().image.type());
                            req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                        } else { // the size matches
                            req->image = req->shot().image;
                            // figure out how long I can afford to wait
                            // For now, 10000 us should be safe
                            Time lockStart = Time::now();
                            if (req->image.lock(10000)) {
                                req->image.copyFrom(Image(req->image.size(), req->image.type(), f->data));
                                req->image.unlock();
                            } else {
                                warning(Event::ImageTargetLocked, sensor,
                                        "Daemon discarding image data (target is still locked, "
                                        "waited for %d us)\n", Time::now() - lockStart);
                                req->image = Image(req->image.size(), req->image.type(), Image::Discard);
                            }
                        }
                    }

                    v4l2Sensor->releaseFrame(f);
                    req->histogram = v4l2Sensor->getHistogram(req->exposureEndTime, req->shot().histogram);
                    req->sharpness = v4l2Sensor->getSharpnessMap(req->exposureEndTime, req->shot().sharpness);
                
                    frameQueue.push(req);
                    enforceDropPolicy();

                }
            
                req = NULL;
                f = NULL;

            } else { // more than 10ms early. Perhaps there was a mode switch.
                dprintf(3, "Handler: Received an early mystery frame (%d %d) vs (%d %d), dropping it.\n",
                        f->processingDoneTime.s(), f->processingDoneTime.us(),
                        req->processingDoneTime.s(), req->processingDoneTime.us());
                v4l2Sensor->releaseFrame(f);
                f = NULL;
            }
        
        }
        pthread_mutex_unlock(&cameraMutex);
    
    }


    void Daemon::runAction() {
        dprintf(2, "Action thread running...\n");
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
            dprintf(3, "Action thread: Initiated action %d us after scheduled time\n", before - a.time);
            delete a.action;
        }
    }

}}
 
