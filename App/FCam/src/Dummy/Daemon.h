#ifndef FCAM_DUMMY_DAEMON_H
#define FCAM_DUMMY_DAEMON_H

#include <pthread.h>

#include <FCam/TSQueue.h>
#include <FCam/Dummy/Sensor.h>

namespace FCam { namespace Dummy {

    void *daemon_launch_thread_(void *arg);

    class Daemon {
    public:
        TSQueue<_Frame *> requestQueue;
        TSQueue<_Frame *> frameQueue;
        
        Daemon(Sensor *sensor);
        ~Daemon();

        void launchThreads();
    private:
        Sensor *sensor;
        
        bool stop;

        bool running;
        void run();

        pthread_t simThread;

        friend void *daemon_launch_thread_(void *arg);
    };

}}
#endif
