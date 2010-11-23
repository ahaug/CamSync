#ifndef FCAM_N900_DAEMON_H
#define FCAM_N900_DAEMON_H

#include <queue>
#include <pthread.h>
#include <semaphore.h>

#include "FCam/Frame.h"
#include "FCam/N900/Sensor.h"
#include "FCam/TSQueue.h"
#include "FCam/N900/Frame.h"

#include "V4L2Sensor.h"



namespace FCam { namespace N900 {

    // The daemon acts as a layer over /dev/video0. It accepts frame
    // requests and returns frames that (hopefully) meet those requests.
    class Daemon {
    public:
        // A class for actions slaved to a frame
        class Action {
        public:
            // When should this action run?
            // The run-time will set this value when it knows when the
            // start of the associated exposure is
            Time time;
            FCam::Action *action;
                
            bool operator<(const Action &other) const {
                // I'm lower priority than another action if I occur later
                return time > other.time;
            }
                
            bool operator>(const Action &other) const {
                // I'm higher priority than another action if I occur earlier
                return time < other.time;
            }
        };
            
            
        Daemon(Sensor *sensor);
        ~Daemon();
            
        // enforce a drop policy on the frame queue
        void setDropPolicy(Sensor::DropPolicy p, int f);

        // The user-space puts partially constructed frames on this
        // queue. It is consumed by the setter thread.
        TSQueue<_Frame *> requestQueue;

        // The handler thread puts mostly constructed frames on this
        // queue. It is consumed by user-space.
        TSQueue<_Frame *> frameQueue;

        void launchThreads();

    private:

        // Access to the V4L2 layer of the sensor
        V4L2Sensor *v4l2Sensor;

        // Access to the FCam sensor object
        Sensor *sensor;

        bool stop;

        // The frameQueue may not grow beyond this limit
        size_t frameLimit;
        // What should I do if the frame policy tries to grow beyond the limit
        Sensor::DropPolicy dropPolicy;
        void enforceDropPolicy();   

        // The setter thread puts in flight requests on this queue, which
        // is consumed by the handler thread
        TSQueue<_Frame *> inFlightQueue;
            
        // Sometimes the setter needs to access the camera exclusively
        // (e.g. to do a pipeline flush). This mutex and flag are used for
        // coordinating such things.
        pthread_mutex_t cameraMutex;
        // The setter thread requests a pipeline flush by setting this
        // flag high and waiting on the above mutex.
        bool pipelineFlush;

        // The setter thread also queues up RT actions on this priority
        // queue, which is consumed by the actions thread
        std::priority_queue<Action> actionQueue;
        pthread_mutex_t actionQueueMutex;
        sem_t actionQueueSemaphore;

        // The component of the daemon that sets exposure and gain
        void runSetter();   
        pthread_t setterThread;
        void tickSetter(Time hs_vs);
        bool setterRunning;
        // The current state of the camera
        _Frame current;
        Shot lastGoodShot;

        // The component that handles frames returned by V4L2
        void runHandler();
        pthread_t handlerThread;
        bool handlerRunning;

        // The component that executes RT actions
        void runAction();
        pthread_t actionThread;
        bool actionRunning;

        int daemon_fd;

        // Have the threads been launched?
        bool threadsLaunched;

        friend void *daemon_setter_thread_(void *arg);
        friend void *daemon_handler_thread_(void *arg);
        friend void *daemon_action_thread_(void *arg);
    };

}
}

#endif
