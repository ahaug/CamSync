#include <pthread.h>

#include "FCam/Action.h"
#include "FCam/F2/Sensor.h"

#include "Platform.h"
#include "Daemon.h"
#include "linux/mt9p031.h"
#include "../Debug.h"



namespace FCam { namespace F2 {


    Sensor::Sensor() : FCam::Sensor(), daemon(NULL), shotsPending_(0) {
        pthread_mutex_init(&requestMutex, NULL);
    }

    Sensor::~Sensor() {
        stop();
        pthread_mutex_destroy(&requestMutex);
    }

    void Sensor::start() {
        if (daemon) return;
        daemon = new Daemon(this);
        daemon->launchThreads();
    }

    void Sensor::stop() {
        if (!daemon) return;
        delete daemon;
        daemon = NULL;
    }

    void Sensor::capture(const FCam::Shot &s) {
        Shot F2s(s);
        F2s.id = s.id;
        capture(F2s);
    }

    void Sensor::capture(const Shot &shot) {
        start();

        // Construct initial fields of \ref F2::_Frame 
        _Frame *f = new _Frame;
        f->_shot = shot;        // make a deep copy here as well
        f->_shot.id = shot.id;

        // push the frame to the daemon
        pthread_mutex_lock(&requestMutex);
        shotsPending_++;
        daemon->requestQueue.push(f);
        pthread_mutex_unlock(&requestMutex);
    }

    void Sensor::capture(const std::vector<FCam::Shot> &burst) {
        std::vector<Shot> F2Burst;
        F2Burst.reserve(burst.size());
        for (unsigned int i=0; i < burst.size(); i++ ) {
            F2Burst.push_back(Shot(burst[i])); // Does this make an unneccessary copy?
            F2Burst[i].id = burst[i].id;
        }
        capture(F2Burst);
    }
        
    void Sensor::capture(const std::vector<Shot> &burst) {
        start();

        std::vector<_Frame *> frames;

        for (size_t i = 0; i < burst.size(); i++) {
           
            // Construct initial fields of \ref F2::_Frame 
            _Frame *f = new _Frame;
            f->_shot = burst[i];        // make a deep copy here as well
            f->_shot.id = burst[i].id;

            frames.push_back(f);
        }

        pthread_mutex_lock(&requestMutex);
        for (size_t i = 0; i < frames.size(); i++) {
            shotsPending_++;
            daemon->requestQueue.push(frames[i]);
        }
        pthread_mutex_unlock(&requestMutex);
    }

    void Sensor::stream(const FCam::Shot &s) {
        Shot F2s(s);
        F2s.id = s.id;
        stream(F2s);
    }

    void Sensor::stream(const Shot &shot) {


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
        std::vector<Shot> F2Burst;
        F2Burst.reserve(burst.size());
        for (unsigned int i=0; i < burst.size(); i++ ) {
            F2Burst.push_back(Shot(burst[i])); // Does this make an unneccessary copy?
            F2Burst[i].id = burst[i].id;
        }
        stream(F2Burst);
    }

    void Sensor::stream(const std::vector<Shot> &burst) {
        pthread_mutex_lock(&requestMutex);
        // do a deep copy of the burst
        streamingShot = burst;

        // Now to copy the IDs
        for (unsigned int i=0; i<burst.size(); i++) {
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

    FCam::F2::Frame Sensor::getFrame() {
        // TODO: How should we handle getFrame when the sensor is stopped?
        start();

        _Frame *_f;
        _f = daemon->frameQueue.pull();

        Frame frame(_f);
        FCam::Sensor::tagFrame(frame);
        for (size_t i=0; i< devices.size(); i++) {
            devices[i]->tagFrame(frame);
        }
        shotsPending_--;
        return frame;
    }
        
    
    Size Sensor::minImageSize() const {
        return Size(640, 480);
    } 
    Size Sensor::maxImageSize() const {
        return Size(MT9P031_ACTIVE_WIDTH, MT9P031_ACTIVE_HEIGHT);
    }
    Size Sensor::pixelArraySize() {
        return Size(MT9P031_ARRAY_WIDTH, MT9P031_ARRAY_HEIGHT);
    }
        
    Rect Sensor::activeArrayRect() {
        return Rect(0,0, 
                    MT9P031_ACTIVE_WIDTH, MT9P031_ACTIVE_HEIGHT);
    }         
    Rect Sensor::pixelArrayRect() {
        return Rect(MT9P031_MIN_COL, 
                    MT9P031_MIN_ROW, 
                    MT9P031_ARRAY_WIDTH, 
                    MT9P031_ARRAY_HEIGHT);
    } 
        
    int Sensor::rollingShutterTime(const Shot &s) const {
        // TODO: put better numbers here
        if (s.image.height() > 960) return 77000;
        else return 33000;
    }

    int Sensor::rollingShutterTime(const FCam::Shot &s) const {
        // TODO: put better numbers here
        if (s.image.height() > 960) return 77000;
        else return 33000;
    }

    // the Daemon calls this when it's time for new frames to be queued up
    void Sensor::generateRequest() {
        pthread_mutex_lock(&requestMutex);
        if (streamingShot.size() ) {
            std::vector<_Frame *> frames;
            
            for (size_t i = 0; i < streamingShot.size(); i++) {
               
                // Construct initial sections of F2::_Frame 
                _Frame *f = new _Frame;
                f->_shot = streamingShot[i];        // make a deep copy here as well
                f->_shot.id = streamingShot[i].id;
                
                frames.push_back(f);
            }
            
            for (size_t i = 0; i < frames.size(); i++) {
                shotsPending_++;
                daemon->requestQueue.push(frames[i]);
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
        if (!daemon) return 0;
        return shotsPending_;
    }

    void Sensor::debugTiming(bool enable) {
        start();
        daemon->debugTiming(enable);
    }
  
    unsigned short Sensor::minRawValue() const {return Platform::minRawValue;}
    unsigned short Sensor::maxRawValue() const {return Platform::maxRawValue;}
        
    BayerPattern Sensor::bayerPattern() const {return Platform::bayerPattern;}        

    const std::string &Sensor::manufacturer() const {return Platform::manufacturer;}

    const std::string &Sensor::model() const {return Platform::model;}        

    void Sensor::rawToRGBColorMatrix(int kelvin, float *matrix) const {
        // call the static platform implementation
        Platform::rawToRGBColorMatrix(kelvin, matrix);
    }

}}
