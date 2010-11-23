#ifndef FCAM_N900_BUTTON_LISTENER_H
#define FCAM_N900_BUTTON_LISTENER_H

#include <pthread.h>

#include "FCam/Event.h"

namespace FCam { 
    namespace N900 {

        class ButtonListener : public EventGenerator {
          public:
            // ButtonListener is a singleton
            static ButtonListener *instance();

          private:
            ButtonListener();
            ~ButtonListener();            

            void run();
 
            pthread_t thread;

            friend void *button_listener_thread_(void *arg);

            bool stop;
        };

    }
}

#endif
