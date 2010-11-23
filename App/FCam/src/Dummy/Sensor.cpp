#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#include <FCam/Event.h>
#include <FCam/Action.h>
#include <FCam/Dummy/Sensor.h>
#include <FCam/Dummy/Platform.h>


#include "Daemon.h"
#include "../Debug.h"


namespace FCam { namespace Dummy {

    Sensor::Sensor(): FCam::Sensor(), daemon(NULL), shotsPending_(0) {
        dprintf(DBG_MINOR, "Initializing dummy simulator sensor.\n");
        pthread_mutex_init(&requestMutex, NULL);
    }

    Sensor::~Sensor() {
        dprintf(DBG_MINOR, "Destroying dummy simulator sensor.\n");
        stop();
        pthread_mutex_destroy(&requestMutex);
    }

    void Sensor::capture(const FCam::Shot &s) {
        Shot dummyShot(s);
        dummyShot.id = s.id;
        capture(dummyShot);
    }

    void Sensor::capture(const Shot &shot) {
        dprintf(DBG_MINOR, "Queuing capture request.\n");
        start();

        _Frame *f = new _Frame;
        f->_shot = shot;
        f->_shot.id = shot.id;

        pthread_mutex_lock(&requestMutex);
        shotsPending_++;
        daemon->requestQueue.push(f);
        pthread_mutex_unlock(&requestMutex);

        daemon->launchThreads();
    }

    void Sensor::capture(const std::vector<FCam::Shot> &burst) {
        std::vector<Shot> dummyBurst;
        dummyBurst.reserve(burst.size());
        for (unsigned int i=0; i < burst.size(); i++ ) {
            dummyBurst.push_back(Shot(burst[i]));
            dummyBurst[i].id = burst[i].id;
        }
        capture(dummyBurst);
    }

    void Sensor::capture(const std::vector<Shot> &burst) {
        dprintf(DBG_MINOR, "Queuing capture request burst.\n");
        start();

        std::vector<_Frame *> frames;

        for (size_t i=0; i < burst.size(); i++) {
            _Frame *f = new _Frame;
            f->_shot = burst[i];
            f->_shot.id = burst[i].id;

            frames.push_back(f);
        }

        pthread_mutex_lock(&requestMutex);
        for (size_t i=0; i < frames.size(); i++) {
            shotsPending_++;
            daemon->requestQueue.push(frames[i]);
        }
        pthread_mutex_unlock(&requestMutex);

        daemon->launchThreads();
    }

    void Sensor::stream(const FCam::Shot &s) {
        Shot dummyShot(s);
        dummyShot.id = s.id;
        stream(dummyShot);
    }

    void Sensor::stream(const Shot &shot) {
        dprintf(DBG_MINOR, "Configuring streaming shot.\n");
        pthread_mutex_lock(&requestMutex);
        streamingShot.clear();
        streamingShot.push_back(shot);
        streamingShot[0].id = shot.id;
        pthread_mutex_unlock(&requestMutex);

        start();
        if (daemon->requestQueue.size() == 0) capture(streamingShot);
    }

    void Sensor::stream(const std::vector<FCam::Shot> &burst) {
        std::vector<Shot> dummyBurst;
        dummyBurst.reserve(burst.size());
        for (unsigned int i=0; i < burst.size(); i++ ) {
            dummyBurst.push_back(burst[i]);
            dummyBurst[i].id = burst[i].id;
        }
        stream(dummyBurst);
    }

    void Sensor::stream(const std::vector<Shot> &burst) {
        dprintf(DBG_MINOR, "Configuring streaming burst.\n");
        pthread_mutex_lock(&requestMutex);
        streamingShot = burst;
        for (size_t i=0; i < burst.size(); i++) {
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
        dprintf(DBG_MINOR, "Stopping streaming.\n");
        pthread_mutex_lock(&requestMutex);
        streamingShot.clear();
        pthread_mutex_unlock(&requestMutex);
    }

    void Sensor::start() {
        dprintf(4, "Creating and launching daemon.\n");
        if (daemon) return;
        daemon = new Daemon(this);
        if (streamingShot.size()) daemon->launchThreads();
        dprintf(4, "Daemon created.\n");
    }

    void Sensor::stop() {
        dprintf(4, "Destroying daemon.\n");
        if (!daemon) return;
        delete daemon;
        daemon = NULL;
    }

    FCam::Dummy::Frame Sensor::getFrame() {
        dprintf(DBG_MINOR, "Getting a frame.\n");
        if (!daemon) {
            Frame invalid;
            error(Event::SensorStoppedError, this, "Can't request frame before calling capture or stream\n");
            return invalid;
        }

        _Frame *_f;
        _f = daemon->frameQueue.pull();

        Frame frame(_f);

        shotsPending_--;

        dprintf(DBG_MINOR, "Frame received.\n");
        return frame;
    }

    int Sensor::rollingShutterTime(const FCam::Shot &s) const {
        return 0;
    }
    
    void Sensor::generateRequest() {
        dprintf(4, "GenerateRequest called by daemon.\n");
        pthread_mutex_lock(&requestMutex);
        if (streamingShot.size() ) {
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
        
    }

    int Sensor::framesPending() const {
        if (!daemon) return 0;
        return daemon->frameQueue.size();
    }

    int Sensor::shotsPending() const {
        return shotsPending_;
    }

}}
