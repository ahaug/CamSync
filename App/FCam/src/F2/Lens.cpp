#include <stdexcept>
#include <sstream>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "FCam/F2/Lens.h"
#include "FCam/Frame.h"

#include "../Debug.h"

/** \todo Fix bugs in RS-232 communication */


/** \todo: Replace with proper error handling */
#define eprintf(...)                                    \
    fprintf(stderr,"EF232[%s]: ERROR! ", tty.c_str());  \
    fprintf(stderr, __VA_ARGS__);


namespace FCam { namespace F2 {

    ////////////////////
    // Public methods
    //

    Lens::Lens(const std::string &tty_): 
        tty(tty_), 
        lensDB(),
        currentLens(NULL),
        state(NotInitialized),
        lensHistory(640),
        controlRunning(false)
    {
        launchControlThread();  
    }

    Lens::~Lens() {
        dprintf(DBG_MINOR,"Lens controller shutting down...\n");
        if (controlRunning) {
            LensOrder shutdown;
            shutdown.cmd = Shutdown;    
            cmdQueue.push(shutdown);
            dprintf(DBG_MINOR,"Waiting for control thread to complete...\n");
            pthread_join(controlThread, NULL);
        }
        dprintf(DBG_MINOR,"Lens controller exit.\n");
    }

    void Lens::setFocus(float diopters, float speed) {
        dprintf(DBG_MINOR,"Setting lens focus to %f\n", diopters);
        LensOrder ord = {SetFocus,
                         diopToEncoder(diopters)};
        cmdQueue.push(ord);
    }
  
    float Lens::getFocus() {
        int val;
        pthread_mutex_lock(&stateMutex);
        val = calcFocusDistance(Time::now());
        pthread_mutex_unlock(&stateMutex);
        return encToDiopter(val);
    }

    float Lens::farFocus() {
        return 0;
    }

    float Lens::nearFocus() {
        LensState s = getState();
        if (s == NotInitialized ||
            s == NoLens) {
            eprintf("maxFocus: Lens not ready!\n");
            return -1;
        }
        return 1000.0/currentLens->focusDistMin;
    }

    bool Lens::focusChanging() {
        return getState() == MovingFocus;
    }

    int Lens::focusLatency() {
        return 0;  /** \todo: calibrate this */
    }

    float Lens::minFocusSpeed() {
        LensState s = getState();
        if (s == NotInitialized ||
            s == NoLens) {
            eprintf("minFocusSpeed: Lens not ready!\n");
            return -1;
        }    
        return currentLens->focusSpeed;
    }

    float Lens::maxFocusSpeed() {
        LensState s = getState();
        if (s == NotInitialized ||
            s == NoLens) {
            eprintf("maxFocusSpeed: Lens not ready!\n");
            return -1;
        }    
        return currentLens->focusSpeed;
    }
   
    void Lens::setZoom(float focal_length_mm, float speed) {
        eprintf("setZoom: Manual zoom only, not zooming to %f\n", focal_length_mm);
    }

    float Lens::getZoom() {
        int val;
        pthread_mutex_lock(&stateMutex);
        val = calcFocalLength(Time::now());
        pthread_mutex_unlock(&stateMutex);
        return val;
        
    }

    float Lens::minZoom() {
        LensState s = getState();
        if (s == NotInitialized ||
            s == NoLens) {
            eprintf("minZoom: Lens not ready!\n");
            return -1;
        }    
        return currentLens->focalLengthMin;
    }
    float Lens::maxZoom() {
        LensState s = getState();
        if (s == NotInitialized ||
            s == NoLens) {
            eprintf("minZoom: Lens not ready!\n");
            return -1;
        }    
        return currentLens->focalLengthMax;
    }

    bool Lens::zoomChanging() {
        return false;
    }

    float Lens::minZoomSpeed() {
        return 0;
    }

    float Lens::maxZoomSpeed() {
        return 0;
    }

    int Lens::zoomLatency() {
        return 0;
    }

    void Lens::setAperture(float f_number, float speed) {
        dprintf(DBG_MINOR,"Setting lens aperture to %f\n", f_number);
        LensOrder ord = {SetAperture,
                         f_number*10};
        cmdQueue.push(ord);
    }

    float Lens::getAperture() {
        int val;
        pthread_mutex_lock(&stateMutex);
        val = calcAperture(Time::now());
        pthread_mutex_unlock(&stateMutex);
        return val/10.0;
    }

    float Lens::wideAperture(float focal_length_mm) {
        LensState s = getState();
        if (s == NotInitialized ||
            s == NoLens) {
            eprintf("minAperture: Lens not ready!\n");
            return -1;
        }    
        
        return currentLens->minApertureAt(focal_length_mm)/10.0;
    }


    float Lens::narrowAperture(float focal_length_mm) {
        LensState s = getState();
        if (s == NotInitialized ||
            s == NoLens) {
            eprintf("maxAperture: Lens not ready!\n");
            return -1;
        }    
        return currentLens->apertureMax/10.0;
    }

    bool Lens::apertureChanging() {
        return getState() == MovingAperture;
    }

    int Lens::apertureLatency() {
        return 0; //! \todo needs to be measured
    }

    float Lens::minApertureSpeed() {
        return 0; //! \todo needs to be measured
    }
       
    float Lens::maxApertureSpeed() {
        return 0; //! \todo needs to be measured
    }
    
    void Lens::reset() {
        LensOrder ord = {
            Initialize,
            0
        };
        
        cmdQueue.push(ord);
    }

    void Lens::tagFrame(FCam::Frame f) {
        pthread_mutex_lock(&stateMutex);

        float initialFocus = encToDiopter(calcFocusDistance(f.exposureStartTime()));
        float finalFocus = encToDiopter(calcFocusDistance(f.exposureEndTime()));
        f["initialFocus"] = initialFocus;
        f["finalFocus"] = finalFocus;
        f["focus"] =  (initialFocus + finalFocus)/2;
        f["focusSpeed"] = (1000000.0f * (finalFocus - initialFocus)/
                           (f.exposureEndTime() - f.exposureStartTime()));
        dprintf(5, "Focus start %f, end %f\n", initialFocus, finalFocus);

        float initialZoom = calcFocalLength(f.exposureStartTime());
        float finalZoom = calcFocalLength(f.exposureEndTime());
        f["initialZoom"] = initialZoom;
        f["finalZoom"] = finalZoom;
        f["zoom"] = (initialZoom + finalZoom)/2;
        f["zoomSpeed"] = 1e6 * (finalZoom - initialZoom)/
            (f.exposureEndTime() - f.exposureStartTime());

        float initialAperture = 10*calcAperture(f.exposureStartTime());
        float finalAperture =  10*calcAperture(f.exposureEndTime());
        f["initialAperture"] = initialAperture;
        f["finalAperture"] = finalAperture;
        f["aperture"] = (initialAperture + finalAperture)/2.;
        f["apertureSpeed"] = 1e6*(finalAperture - initialAperture) /
            (f.exposureEndTime() - f.exposureStartTime());
       
        pthread_mutex_unlock(&stateMutex);
    }


    ////////////////////
    // Private methods
    //

    void Lens::launchControlThread() {
        pthread_mutex_init(&stateMutex, NULL);
        if (pthread_create(&controlThread, NULL, lensControlThread, this) != 0) {
            perror("Error creating lens control thread");
        } else {
            controlRunning = true; 
        }
    }

    // Static member function helper to launch a thread-running method.
    void *Lens::lensControlThread(void *arg) {
        Lens *l = (Lens *)arg;
        l->runLensControlThread();
        pthread_exit(NULL);
    }

    void Lens::runLensControlThread() {
        bool done = false;
        dprintf(DBG_MINOR,"Lens control thread starting\n");
        init();
        while(!done) {
            // To start, let's see if anyone wants us to do anything
            // over the next 200 ms
            bool cmdReady;
            cmdReady = cmdQueue.wait(200000);
            if (!cmdReady) {
                dprintf(5,"LCT: idle\n");
                if (getState() == NotInitialized) continue;
                // Idle processing, then, if the lens is initialized
                idleProcessing();
                
            } else {
                LensOrder ord = cmdQueue.front();
                cmdQueue.pop();
                dprintf(5,"LCT: %d %d\n", ord.cmd, ord.val);
                switch(ord.cmd) {
                case Initialize:
                    init();
                    break;
                case Calibrate:
                    calibrateLens();
                    break;
                case SetAperture: 
                    {
                        setState(MovingAperture);
                        LensParams params;
                        params.time=Time::now();
                        params.focalLength = lensHistory[0].focalLength;
                        params.aperture = lensHistory[0].aperture;
                        params.focusDist = lensHistory[0].focusDist;
                        lensHistory.push(params);
                    
                        unsigned int newAperture = cmd_DoAperture(ord.val);
                    
                        params.aperture = newAperture;
                        params.time = Time::now();
                        lensHistory.push(params);
                        setState(Ready);
                    } break;
                case SetFocus: 
                    {
                        setState(MovingFocus);
                        LensParams params;
                        params.time=Time::now();
                        params.focalLength = lensHistory[0].focalLength;
                        params.aperture = lensHistory[0].aperture;
                        params.focusDist = lensHistory[0].focusDist;
                        lensHistory.push(params);

                        unsigned int newFocusDist = cmd_DoFocus(ord.val);
                        params.focusDist = newFocusDist;
                        params.time = Time::now();
                        lensHistory.push(params);
                        setState(Ready);
                    } break;
                case Shutdown:
                    done = true;
                    break;
                }                   
            }
        }
        close(serial_fd);
        dprintf(DBG_MINOR,"Lens control thread shutting down\n");
    }

    void Lens::idleProcessing() {
        // Should handle lens re-detection here
        if (getState() != Ready) {
            //eprintf("getZoom: Lens not ready!\n");
            return;
        }
        setState(Busy);
        std::string idStr = cmd_GetID();
        unsigned int fl = cmd_GetFocalLength(idStr);
        if (fl != lensHistory[0].focalLength) {
            LensParams currentParams;
            currentParams.focalLength = fl;
            currentParams.focusDist = lensHistory[0].focusDist;
            currentParams.time = Time::now();

            unsigned int minAperture = cmd_GetMinAperture(idStr);
            if (minAperture > lensHistory[0].aperture) {
                currentParams.aperture = minAperture;
            } else {
                currentParams.aperture = lensHistory[0].aperture;
            }

            lensHistory.push(currentParams);
            
            if (currentLens->minApertureAt(lensHistory[0].focalLength) != minAperture) {
                dprintf(5,"Adding entry to min aperture map: %d->%d, currently %d\n", 
                        lensHistory[0].focalLength, minAperture, 
                        currentLens->minApertureAt(lensHistory[0].focalLength));
                // Update mapping
                EF232LensInfo updatedInfo = *currentLens;
                EF232LensInfo::minApertureListIter iter=
                    updatedInfo.
                    minApertureList.
                    insert(updatedInfo.minApertureList.begin(),
                           EF232LensInfo::apertureChange(lensHistory[0].focalLength,
                                                         minAperture));
                iter++;
                while (iter != updatedInfo.minApertureList.end()) {
                    if (iter->second == minAperture) {
                        dprintf(5,"Removing redundant entry %d->%d\n", iter->first, iter->second);
                        updatedInfo.minApertureList.erase(iter++);
                    } else {
                        iter++;
                    }
                }       
                currentLens = lensDB.update(updatedInfo);       
            }
        }
        setState(Ready);

        return;
    }

    void Lens::setState(LensState newState) {
        pthread_mutex_lock(&stateMutex);
        state = newState;
        pthread_mutex_unlock(&stateMutex);
    }

    Lens::LensState Lens::getState() {
        LensState curState;
        pthread_mutex_lock(&stateMutex);
        curState = state;
        pthread_mutex_unlock(&stateMutex);
        return curState;
    }


    int Lens::calcFocusDistance(const Time &t) const {
        unsigned int index =0;
        while (index < lensHistory.size() &&
               lensHistory[index].time >= t) {
            index++;
        }
        if (index == 0) {
            return lensHistory[0].focusDist;
        }
        if (index == lensHistory.size()) {
            return lensHistory[lensHistory.size()-1].focusDist;
        }
        // Linear interpolation between two entries, in diopter space
        int fStart = lensHistory[index].focusDist;
        int fEnd = lensHistory[index-1].focusDist;
        if (fStart == fEnd) return fStart;

        float tfrac = (t - lensHistory[index].time) 
            / ((float) (lensHistory[index-1].time - lensHistory[index].time));
        
        return fStart * tfrac + fEnd * (1-tfrac);
    }

    int Lens::calcAperture(const Time &t) const {
        unsigned int index =0;
        while (index < lensHistory.size() &&
               lensHistory[index].time >= t) {
            index++;
        }
        if (index == 0) {
            return lensHistory[0].aperture;
        }
        if (index == lensHistory.size()) {
            return lensHistory[lensHistory.size()-1].aperture;
        }

        int aStart = lensHistory[index].aperture;
        int aEnd = lensHistory[index-1].aperture;
        if (aStart == aEnd) return aStart;

        float tfrac = (t - lensHistory[index].time) 
            / ((float) (lensHistory[index-1].time - lensHistory[index].time));
        
        return aStart * tfrac + aEnd * (1-tfrac);
    }

    int Lens::calcFocalLength(const Time &t) const {
        unsigned int index =0;
        while (index < lensHistory.size() &&
               lensHistory[index].time >= t) {
            index++;
        }
        if (index == 0) {
            return lensHistory[0].focalLength;
        }
        if (index == lensHistory.size()) {
            return lensHistory[lensHistory.size()-1].focalLength;
        }

        int fStart = lensHistory[index].focalLength;
        int fEnd = lensHistory[index-1].focalLength;
        if (fStart == fEnd) return fStart;

        float tfrac = (t - lensHistory[index].time) 
            / ((float) (lensHistory[index-1].time - lensHistory[index].time));
        
        return fStart * tfrac + fEnd * (1-tfrac);

    }

    //////
    // Diopter->encoder conversions for focus distance
    float Lens::encToDiopter(unsigned int encoder) {
        return diopScaleFactor * (focusEncoderMax - encoder);
    }

    unsigned int Lens::diopToEncoder(float diopters) {
        return focusEncoderMax - (diopters/diopScaleFactor);
    }

    //////
    // Needed forward declaration of template specialization
    template<>
    void Lens::read(std::string &val);

    //////
    // Primary initialization for the lens

    void Lens::init() {
        ////
        // Open serial port device
        serial_fd = open (tty.c_str(), 
                          O_RDWR | O_NOCTTY | O_NDELAY);

        if (serial_fd < 0) {
            eprintf("Unable to open serial device!");
            return;
        }
    
        ////
        // Configure port communication parameters

        // reads will block
        fcntl(serial_fd, F_SETFL, 0);

        struct termios opts;
        tcgetattr(serial_fd, &opts);
        // set speed to 19200 8n1
        opts.c_cflag = B19200 | CS8 | CLOCAL | CREAD;
        opts.c_lflag = 0;
        opts.c_iflag = 0;
        opts.c_oflag = 0;
        // set the read timeout to 0.1 seconds
        opts.c_cc[VMIN] = 0;
        opts.c_cc[VTIME] = 1;
        // write the opts struct
        tcflush(serial_fd, TCIFLUSH);
        tcsetattr(serial_fd, TCSANOW, &opts);

        // Set lens starting parameters

        try {
            cmd_DoInitialize();
            cmd_DoApertureOpen();
     
            state = NoLens;
        } catch (std::runtime_error err) {
            eprintf("Unable to initialize the lens controller!\n");
            close(serial_fd);
            state = NotInitialized;
            return;
        }        

        calibrateLens();
    }

    //////
    // Detect and measure lens parameters

    void Lens::calibrateLens() {
        try {
            unsigned int focalMin, focalMax;

            int focalLength = cmd_GetFocalLength();

            cmd_GetZoomRange(focalMin, focalMax);

            // Find lens in database, or create a new entry for it 
            currentLens = lensDB.find(focalMin, focalMax);

            // Focus near, and reset lens focus distance encoder 
            cmd_DoFocusAtZero();
            cmd_SetFocusEncoder(0);
      
            // Find minimum focus distance if not known
            if (currentLens->focusDistMin == 0) {
                dprintf(5,"Measuring minimum focus distance\n");
                unsigned int newFDMin, newFDMax;

                cmd_GetFocusBracket(newFDMin, newFDMax);
                if (newFDMin == 0) {
                    // Can't find out what the minimum focus distance for this lens is
                    // And we don't have it in the DB, either.
                    // This results in a lot of buggy math down the line, FIXME
                    eprintf("Unknown lens minimum focus distance, assuming 0mm!\n");
                } else {
                    EF232LensInfo updatedInfo = *currentLens;
                    updatedInfo.focusDistMin = newFDMin;
                    currentLens = lensDB.update(updatedInfo);
                    dprintf(5, "Updated focusDistMin to %d\n", currentLens->focusDistMin);
                }
            }

            // Find maximum focus encoder count, assume that's infinity
            // Note - that assumption is a bit dangerous, should really calibrate better
            cmd_DoFocusAtInf();

            focusEncoderMax = cmd_GetFocusEncoder();
            diopScaleFactor = 1000.0/(currentLens->focusDistMin
                                      * focusEncoderMax);

            state = Ready;

            if (currentLens->apertureMax == 0) {
                dprintf(DBG_MINOR,"Measuring max aperture...\n");
                unsigned int newApertureMax = cmd_DoApertureClose();
                EF232LensInfo updatedInfo = *currentLens;
                updatedInfo.apertureMax = newApertureMax;
                currentLens = lensDB.update(updatedInfo);     
            }

            if (currentLens->focusSpeed == 0) {
                dprintf(DBG_MINOR,"Measuring focus speed...\n");
                // Roughly measure focusing speed
                std::vector<float> focusSpeeds;
                float maxDiop = 1000.0/currentLens->focusDistMin;
                for (unsigned int percent = 10; percent <= 100; percent+=20) {
                    timeval startFocus, midFocus, endFocus;
                    float destDiop = maxDiop*percent/100;
                    gettimeofday(&startFocus, NULL);
                    setFocus(destDiop);
                    gettimeofday(&midFocus, NULL);
                    setFocus(0);
                    gettimeofday(&endFocus, NULL);
          
                    unsigned int infToZeroT = (midFocus.tv_sec - startFocus.tv_sec)*1000000 
                        + (midFocus.tv_usec - startFocus.tv_usec);
                    unsigned int zeroToInfT = (endFocus.tv_sec - midFocus.tv_sec)*1000000 
                        + (endFocus.tv_usec - midFocus.tv_usec);
  
                    float infToZeroDPS = (destDiop/(float)infToZeroT)*1e6;
                    float zeroToInfDPS = (destDiop/(float)zeroToInfT)*1e6;
                    dprintf(5, "Focus Speed calib for 0 to %f diop: %f diop/sec inf-to-zero, %f diop/sec zero-to-inf\n", destDiop, infToZeroDPS, zeroToInfDPS);
                    focusSpeeds.push_back(infToZeroDPS);
                    focusSpeeds.push_back(zeroToInfDPS);
                }

                // Get median speed, more or less
                std::sort(focusSpeeds.begin(), focusSpeeds.end());
                float newFocusSpeed = focusSpeeds[(focusSpeeds.size()+1)/2];
                dprintf(5, "Setting focus speed to median: %f DPS\n", newFocusSpeed);
        
                EF232LensInfo updatedInfo = *currentLens;
                updatedInfo.focusSpeed = newFocusSpeed;
                currentLens = lensDB.update(updatedInfo);
        
            }
      
            if (currentLens->minApertureList.size() == 0) {
                dprintf(DBG_MINOR,"Establishing minimal focal length->min aperture map\n");
                // Minimal entry for min aperture/focal length mapping
                unsigned int newFocalLength = cmd_GetFocalLength();
                unsigned int newMinAperture = cmd_GetMinAperture();
                EF232LensInfo updatedInfo = *currentLens;
                updatedInfo.minApertureList[newFocalLength] = newMinAperture;
                currentLens = lensDB.update(updatedInfo);
            }

            int aperture = cmd_DoApertureOpen();
            int focusDistance = cmd_GetFocusEncoder();

            LensParams calibParams;
            calibParams.time = Time::now();
            calibParams.focalLength = focalLength;
            calibParams.aperture = aperture;
            calibParams.focusDist= focusDistance;
            lensHistory.push(calibParams);

           
        } catch (std::runtime_error err) {
            eprintf("Unable to configure lens!\n");
            state = NoLens;
        }
    }

    //////
    // Individual command executers

    std::string Lens::cmd_GetID() {
        send("id");
        expect("OK");
        read(buf); // Expecting a string of form "NNmm,fNN"
        dprintf(6,"* ID response: %s\n", buf.c_str());
        return buf;
    }

    unsigned int Lens::cmd_GetFocalLength(std::string idStr) {
        std::stringstream parsebuf;
        if (idStr == "") idStr = cmd_GetID();
        parsebuf.str(idStr);
        unsigned int val;
        parsebuf >> val;
        return val;
    }

    unsigned int Lens::cmd_GetMinAperture(std::string idStr) {
        std::stringstream parsebuf;
        if (idStr == "") idStr = cmd_GetID();
        parsebuf.str(idStr);
        unsigned int val;
        parsebuf.ignore(10,'f');
        parsebuf >> val;
        return val;
    }

    void Lens::cmd_GetZoomRange(unsigned int &min,
                                unsigned int &max) {
        std::stringstream parsebuf;
        send("dz"); // Get lens zoom range
        expect("OK");
        read(buf); // Expecting a string of form "NNmm,NNmm"
        parsebuf.str(buf);
        parsebuf >> min;
        parsebuf.ignore(10,',');
        parsebuf >> max;
        dprintf(6,"* DZ response: %s\n",buf.c_str());
    }

    void Lens::cmd_GetFocusBracket(unsigned int &min,
                                   unsigned int &max) {
        std::stringstream parsebuf;
        send("fd"); // See if we can get a focus distance bracket here at zero focus
        expect("OK");
        read(buf); // Expecting NNcm,NNcm
        parsebuf.str(buf);
        parsebuf >> min;
        parsebuf.ignore(10,',');
        parsebuf >> max;
        min*=10;
        max*=10;    
        dprintf(6,"* FD response: %s\n", buf.c_str());
    }

    Lens::LensError::e Lens::cmd_DoInitialize() {
        send("in");  
        expect("OK");
        LensError::e err = errorCheck(buf);
        if (err == LensError::None) { 
            dprintf(6,"* IN successful\n");
        } else {
            dprintf(6,"* IN Failure!\n");
        }
        return err;
    }

    unsigned int Lens::cmd_DoApertureOpen() {
        std::stringstream parsebuf;
        unsigned int val;
        send("mo");
        expect("OK");
        expect("DONE", buf); // Expecting DONE<steps>,f<f_number*10>
        dprintf(6,"* MO response: %s\n", buf.c_str());
        parsebuf.str(buf);
        parsebuf.ignore(10,'f');
        parsebuf >> val;
        return val;
    }

    unsigned int Lens::cmd_DoAperture(unsigned int val) {
        std::stringstream parsebuf;
        unsigned int minAperture = currentLens->minApertureAt(lensHistory[0].focalLength);
        unsigned int newVal;

        // Aperture position encoded as 1/4-stops from fully-open
        unsigned int aperture_enc = (val-minAperture)*4/10;
        parsebuf << "ma" << aperture_enc;
        dprintf(6,"* MA send: %s\n", parsebuf.str().c_str());
        send(parsebuf.str());
        expect("OK");
        expect("DONE", buf); // Expecting DONE<step>,f<f_number*10>

        dprintf(6,"* MA response: %s\n", buf.c_str());
        parsebuf.str(buf);
        parsebuf.ignore(10,'f');
        parsebuf >> newVal;
        return newVal;
    }

    unsigned int Lens::cmd_DoApertureClose() {
        std::stringstream parsebuf;
        unsigned int val;
        send("mc");
        expect("OK");
        expect("DONE", buf); // Expecting DONE<steps>,f<f_number*10>
        dprintf(6,"* MC response: %s\n", buf.c_str());
        parsebuf.str(buf);
        parsebuf.ignore(10,'f');
        parsebuf >> val;
        return val;
    }

    void Lens::cmd_DoFocusAtZero() {
        send("mz"); // Set lens focus distance to nearest
        expect("OK");
        expect("DONE", buf);
        dprintf(6,"* MZ response: %s\n", buf.c_str());
    }

    unsigned int Lens::cmd_DoFocus(unsigned int val) {
        std::stringstream parsebuf, parsebuf2;
        unsigned int encVal;
        parsebuf << "fa" << val;
        send(parsebuf.str());
        expect("OK");
        expect("DONE", buf);
        parsebuf2.str(buf);
        parsebuf2 >> encVal;
        dprintf(6,"* FA response: %s (%d)\n", buf.c_str(), encVal);
        return encVal;
    }

    void Lens::cmd_DoFocusAtInf() {
        send("mi"); // Set lens focus distance to infinity
        expect("OK");
        expect("DONE", buf);
        dprintf(6,"* MI response: %s\n", buf.c_str());    
    }

    void Lens::cmd_SetFocusEncoder(unsigned int val) {
        std::stringstream parsebuf;
        parsebuf << "sf" << val;
        send(parsebuf.str());
        expect("OK");
        dprintf(6,"* SF successful\n");
    }

    unsigned int Lens::cmd_GetFocusEncoder() {
        std::stringstream parsebuf;
        int val;
        send("pf"); // Read encoder value at focus position
        expect("OK");
        read(buf);
        dprintf(6,"* PF response: %s\n", buf.c_str());
        parsebuf.str(buf);
        parsebuf >> val;
        return val;
    }

    //////
    // Basic serial communication primitives

    void Lens::send(const std::string &str) {
        dprintf(7,"Sending: %s\n", str.c_str());
        char c = 13;
        usleep(1000);
        write(serial_fd, str.c_str(), str.size());
        write(serial_fd, &c, 1);

        expect(str);
    }

    template<>
    void Lens::read(std::string &str) {   
        char nextC;
        int timeoutCounter = 0;
        str.clear();
        do {
            int ret = ::read(serial_fd, &nextC, 1);
            if (ret > 0 && nextC != 13) {
                str += nextC; 
            } else if (ret < 0) {
                eprintf("Read returned %i\n", ret);
                return;
            } else {
                timeoutCounter++;
                if (timeoutCounter == 30) {
                    eprintf("Lost comm with lens controller\n");
                    throw std::runtime_error("timeout communicating with controller");
                }
            }
        } while (nextC != 13);
        dprintf(7,"Received: '%s'\n", str.c_str());
    }

    template<typename T>
    void Lens::read(T &val) {
        std::stringstream str;

        read(str.str());
        str >> val;
    }

    void Lens::expect(const std::string &desired) {
        std::string buf;
        read(buf);
        dprintf(6,"Expect: Got '%s'\n", buf.c_str());
        if (buf.substr(0, desired.size()) == desired) return;

        eprintf("Expected: '%s' Got: '%s'\n", desired.c_str(), buf.c_str());
        throw std::runtime_error("expect mismatch");
    }

    Lens::LensError::e Lens::errorCheck(std::string &remainder) {
        std::string buf;
        read(buf);
        if (buf == "DONE") {        
            remainder = buf.substr(4);
            return LensError::None;
        } else if (buf.substr(0,3) == "ERR") {
            std::stringstream errNumStr(buf.substr(4));
            int errNum;
            errNumStr >> errNum;
            return static_cast<LensError::e>(errNum);
        } else {
            return LensError::UnknownError;
        }       
    }

    void Lens::expect(const std::string &desired, std::string &remainder) {
        std::string buf;
        read(buf);
        if (buf.substr(0, desired.size()) == desired) {
            remainder = buf.substr(desired.size());
            return;
        }
    
        eprintf("Expected: %s\n Got: %s\n", desired.c_str(), buf.c_str());
        throw std::runtime_error("expect mismatch");
    }
    
}}
