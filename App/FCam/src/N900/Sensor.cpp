#include <pthread.h>

#include "FCam/Action.h"
#include "FCam/N900/Sensor.h"

#include "FCam/N900/Platform.h"
#include "Daemon.h"
#include "ButtonListener.h"
#include "../Debug.h"



namespace FCam { namespace N900 {

    Sensor::Sensor() : FCam::Sensor(), daemon(NULL), shotsPending_(0) {
        // make sure the N900 button listener is running
        
        // TODO: put this somewhere better?
        ButtonListener::instance();
        
        pthread_mutex_init(&requestMutex, NULL);
        
    }
    
    Sensor::~Sensor() {
        stop();
        pthread_mutex_destroy(&requestMutex);
    }
    
    void Sensor::start() {        
        if (daemon) return;
        daemon = new Daemon(this);
        if (streamingShot.size()) daemon->launchThreads();
    }

    void Sensor::stop() {
        dprintf(3, "Entering Sensor::stop\n"); 
        stopStreaming();

        if (!daemon) return;

        dprintf(3, "Cancelling outstanding requests\n");

        // Cancel as many requests as possible
        pthread_mutex_lock(&requestMutex);
        _Frame *req;
        while (daemon->requestQueue.tryPullBack(&req)) {
            delete req;
            shotsPending_--;
        }
        pthread_mutex_unlock(&requestMutex);        

        dprintf(3, "Discarding remaining frames\n");

        // Wait for the outstanding ones to complete
        while (shotsPending_) {
            delete daemon->frameQueue.pull();
            decShotsPending();
        }

        dprintf(3, "Deleting daemon\n"); 

        // Delete the daemon
        if (!daemon) return;
        delete daemon;
        daemon = NULL;

        dprintf(3, "Sensor stopped\n"); 
    }
    
    void Sensor::capture(const FCam::Shot &shot) {
        start();
        
        _Frame *f = new _Frame;
        
        // make a deep copy of the shot to attach to the request
        f->_shot = shot;        
        // clone the shot ID
        f->_shot.id = shot.id;

        // push the frame to the daemon
        pthread_mutex_lock(&requestMutex);
        shotsPending_++;
        daemon->requestQueue.push(f);
        pthread_mutex_unlock(&requestMutex);

        daemon->launchThreads();
    }
    
    void Sensor::capture(const std::vector<FCam::Shot> &burst) {
        start();              

        std::vector<_Frame *> frames;
        
        for (size_t i = 0; i < burst.size(); i++) {
            _Frame *f = new _Frame;
            f->_shot = burst[i];
            
            // clone the shot ID
            f->_shot.id = burst[i].id;

            frames.push_back(f); 
        }

        pthread_mutex_lock(&requestMutex);
        for (size_t i = 0; i < frames.size(); i++) {
            shotsPending_++;
            daemon->requestQueue.push(frames[i]);
        }
        pthread_mutex_unlock(&requestMutex);

        daemon->launchThreads();
    }
    
    void Sensor::stream(const FCam::Shot &shot) {
        pthread_mutex_lock(&requestMutex);
        streamingShot.clear();
        // this makes a deep copy of the shot
        streamingShot.push_back(shot);
        streamingShot[0].id = shot.id;
        pthread_mutex_unlock(&requestMutex);

        start();
        if (daemon->requestQueue.size() == 0) capture(streamingShot);
    }
    
    void Sensor::stream(const std::vector<FCam::Shot> &burst) {
        pthread_mutex_lock(&requestMutex);
        
        // do a deep copy of the burst
        streamingShot = burst;
        
        // clone the ids 
        for (size_t i = 0; i < burst.size(); i++) {
            streamingShot[i].id = burst[i].id;
        }
        pthread_mutex_unlock(&requestMutex);

        start();
        if (daemon->requestQueue.size() == 0) capture(streamingShot);
    }
    
    bool Sensor::streaming() {
        return streamingShot.size() > 0;
    }
    
    void Sensor::stopStreaming() {
        pthread_mutex_lock(&requestMutex);
        streamingShot.clear();
        pthread_mutex_unlock(&requestMutex);
    }
        
    Frame Sensor::getFrame() {
        if (!daemon) {
            Frame invalid;
            error(Event::SensorStoppedError, "Can't request a frame before calling capture or stream\n");
            return invalid;
        }        
        Frame frame(daemon->frameQueue.pull());
        FCam::Sensor::tagFrame(frame); // Use the base class tagFrame
        for (size_t i = 0; i < devices.size(); i++) {
            devices[i]->tagFrame(frame);
        }
        decShotsPending();
        return frame;
    }
    
    int Sensor::rollingShutterTime(const Shot &s) const {
        // TODO: put better numbers here
        if (s.image.height() > 960) return 77000;
        else return 33000;
    }
    
    // the Daemon calls this when it's time for new frames to be queued up
    void Sensor::generateRequest() {
        pthread_mutex_lock(&requestMutex);
        if (streamingShot.size()) {
            for (size_t i = 0; i < streamingShot.size(); i++) {
                _Frame *f = new _Frame;
                f->_shot = streamingShot[i];                
                f->_shot.id = streamingShot[i].id;                
                shotsPending_++;
                daemon->requestQueue.push(f);
            }
        }
        pthread_mutex_unlock(&requestMutex);

    }
    
    void Sensor::enforceDropPolicy() {
        if (!daemon) return;
        daemon->setDropPolicy(dropPolicy, frameLimit);
    }
    
    int Sensor::framesPending() const {
        if (!daemon) return 0;
        return daemon->frameQueue.size();
    }
    
    int Sensor::shotsPending() const {
        return shotsPending_;
    }

    void Sensor::decShotsPending() {
        pthread_mutex_lock(&requestMutex);
        shotsPending_--;
        pthread_mutex_unlock(&requestMutex);        
    }
        
}}
