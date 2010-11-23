#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <poll.h>
#include <unistd.h>
#include <sstream>

#include "FCam/Event.h" 
#include "FCam/Time.h" 

#include "ButtonListener.h"
#include "../Debug.h"




namespace FCam { 
    namespace N900 {

        void *button_listener_thread_(void *arg) {
            ButtonListener *b = (ButtonListener *)arg;
            b->run();    
            pthread_exit(NULL);
            return NULL;
        }
        
        ButtonListener *ButtonListener::instance() {
            // Return a pointer to a static function variable. I feel
            // dirty doing this, but it guarantees that 
            // 1) The buttonlistener is created on first use
            // 2) The destructor gets called on program exit

            // See http://www.research.ibm.com/designpatterns/pubs/ph-jun96.txt
            // for an interesting discussion of ways to make sure singletons get deleted.

            static ButtonListener _instance;
            return &_instance;
        }
        
        ButtonListener::ButtonListener() : stop(false) {
            pthread_attr_t attr;
            struct sched_param param;
            
            // make the thread
            
            param.sched_priority = sched_get_priority_max(SCHED_OTHER);            
            
            pthread_attr_init(&attr);
            
            if ((errno =
                 -(pthread_attr_setschedparam(&attr, &param) ||
                   pthread_attr_setschedpolicy(&attr, SCHED_OTHER) ||
                   pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) ||
                   pthread_create(&thread, &attr, button_listener_thread_, this)))) {
                error(Event::InternalError, "Error creating button listener thread");
            }

        }

        ButtonListener::~ButtonListener() {
            stop = true;
            pthread_join(thread, NULL);
        }

        void ButtonListener::run() {
            // TODO: The below is a horrible hack to prevent the built-in
            // camera program from stealing shutter press events. I really
            // wish there was a better way to do this.
            
            // kill the built-in camera program if it was launched at boot
            if (!fork()) {
                dprintf(2, "Killing camera-ui using dsmetool\n");
                // suppress output
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                execl("/usr/sbin/dsmetool",
                      "/usr/sbin/dsmetool",
                      "-k", "/usr/bin/camera-ui",
                      (char *)NULL);
                exit(0);
            }
            
            if (!fork()) {
                // Give the dsmetool a chance to do it's
                // thing. However, if the camera was launched from the
                // App menu it may not work, so use killall as well.
                usleep(100000);
                dprintf(2, "Killing camera-ui using killall\n");
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                execl("/usr/bin/killall", "/usr/bin/killall", "camera-ui", (char *)NULL);
                exit(0);
            }
            
            // Start a watchdog process that respawns the camera-ui
            // when the fcam program terminates (even if it crashes)

            // Create the file to be locked and lock it.
            int lockFD = open("/tmp/fcam.lock", O_RDWR | O_CREAT);
            int ret = flock(lockFD, LOCK_EX | LOCK_NB);
            if (ret == EWOULDBLOCK) {
                // someone already has this file locked. Must be
                // another fcam process. The user will find out when
                // the try to instantiate a sensor so let's silently
                // bail out here.
                return;                
            }

            if (!fork()) {
                // make my own process group, so signals sent to the
                // parent (eg SIGSEGV or SIGKILL) don't kill me
                // too. Unfortunately this also means I can't just use
                // waitpid.
                setpgrp();
                // close the duplicated lockFD from the parent
                close(lockFD);
                // wait for the parent to terminate by locking the same file
                flock(open("/tmp/fcam.lock", O_RDWR | O_CREAT), LOCK_EX);
                // respawn the camera-ui
                dprintf(2, "Respawning camera-ui");
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                execl("/usr/sbin/dsmetool", 
                      "/usr/sbin/dsmetool", 
                      "-U", "user", 
                      "-o", "/usr/bin/camera-ui", 
                      (char *)NULL);                
            }

            const int BUTTONS = 4;

            const char *fnames[BUTTONS] = {"/sys/devices/platform/gpio-switch/cam_shutter/state",
                                           "/sys/devices/platform/gpio-switch/cam_focus/state",
                                           "/sys/devices/platform/gpio-switch/cam_launch/state",
                                           "/sys/devices/platform/gpio-switch/slide/state"};
                                               
            char buf[81];

            bool state[BUTTONS];

            int event[BUTTONS*2] = {Event::N900LensOpened,
                                    Event::N900LensClosed,
                                    Event::FocusPressed,
                                    Event::FocusReleased,
                                    Event::ShutterPressed,
                                    Event::ShutterReleased,
                                    Event::N900SlideOpened,
                                    Event::N900SlideClosed};

            std::string descriptions[BUTTONS*2] = {"Lens cover opened",
                                                   "Lens cover closed",
                                                   "Focus button pressed",
                                                   "Focus button released",
                                                   "Shutter button pressed",
                                                   "Shutter button released",
                                                   "Keyboard slide opened",
                                                   "Keyboard slide closed"};

            // open all the devices
            int rval;
            struct pollfd fds[BUTTONS];
            for (int i = 0; i < BUTTONS; i++) {
                fds[i].fd = open(fnames[i], O_RDONLY);
                fds[i].events = POLLPRI;
                fds[i].revents = 0;
            }

            // read the initial state
            for (int i = 0; i < BUTTONS; i++) {
                rval = read(fds[i].fd, &buf, 80);
                buf[rval] = 0;
                switch (buf[0]) {
                case 'c': // closed
                case 'i': // inactive
                    state[i] = false;
                    break;
                case 'o': // open
                case 'a': // active
                    state[i] = true;
                    break;
                default:                    
                    error(Event::InternalError, "ButtonListener: Unknown state: %s", buf);
                }
            }            

            while (!stop) {               
                // wait for a change
                rval = poll(fds, BUTTONS, 1000);
                if (rval == -1) {
                    // this fails once on load, not sure why
                    dprintf(2, "ButtonListener: poll failed");
                    //error(Event::InternalError, "ButtonListener: poll failed");
                    continue;
                }
                if (rval == 0) continue; // timeout

                for (int i = 0; i < BUTTONS; i++) {
                    if (fds[i].revents & POLLPRI) {
                        close(fds[i].fd);
                        fds[i].fd = open(fnames[i], O_RDONLY, 0);
                        rval = read(fds[i].fd, &buf, 80);
                        buf[rval] = 0;
                        switch (buf[0]) {
                        case 'c': // closed
                        case 'i': // inactive
                            if (state[i] != false) {
                                state[i] = false;
                                postEvent(event[i*2+1], 0, descriptions[i*2+1]);
                            }
                            break;
                        case 'o': // open
                        case 'a': // active
                            if (state[i] != true) {
                                state[i] = true;
                                postEvent(event[i*2], 0, descriptions[i*2]);
                            }
                            break;
                        default:
                            error(Event::InternalError, "ButtonListener: Unknown state: %s", buf);
                        }
                    }
                }                
            }

            dprintf(2, "Button listener shutting down\n");

            for (int i = 0; i < BUTTONS; i++) {
                close(fds[i].fd);
            }
            close(lockFD);
        }
    }
}


