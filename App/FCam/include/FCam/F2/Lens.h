#ifndef EF232LENS_H
#define EF232LENS_H

/** \file 
 * The lens controller device for the F2. 
 * Communicates with the Birger EF-232 Canon EOS lens controller over RS-232.
 */

#include "../Lens.h"
#include "../TSQueue.h"
#include "../CircularBuffer.h"

#include <string>

#include "EF232LensDatabase.h"

namespace FCam { namespace F2 {

    /** The F2 Lens device. The device communicates with the Birger
     * Engineering EF-232 Canon EOS lens controller, over the serial
     * port. The class runs a separate control thread to avoid
     * blocking on slow RS-232 transactions. There is no way to
     * programmatically zoom Canon EOS lenses, although their focal
     * length can be manually changed.  This means that changing zoom
     * is impossible, but it may change on its own due to user
     * actions.  Similarly, focus may be altered manually by the user,
     * and if the user toggles on manual focus, it is impossible to
     * focus automatically.  The lenses are also removable, so the
     * lens controller may not be able to do anything, if no lens is
     * connected to it.
     * 
     * \todo Hook up events for user-made changes to focal length,
     * focus distance, and lens detach/attach.
     *
     * \todo Expose image stabilization to the API
     */
    class Lens: public FCam::Lens {
 
    public:
        Lens(const std::string &tty_ = "/dev/ttyS2");
        ~Lens();
   
        void setFocus(float diopters, float speed = -1);
        float getFocus();
        float farFocus();
        float nearFocus();
        bool focusChanging();
        int focusLatency();

        float minFocusSpeed();
        float maxFocusSpeed();
    
        void setZoom(float focal_length_mm, float speed = -1);
        float getZoom();
        float minZoom();
        float maxZoom();
        bool zoomChanging();
        int zoomLatency();
        float minZoomSpeed();
        float maxZoomSpeed();
    
        void setAperture(float f_number, float speed = -1);
        float getAperture();
        float wideAperture(float focal_length_mm = -1);
        float narrowAperture(float focal_length_mm = -1);
        bool apertureChanging();
        int apertureLatency();
        float minApertureSpeed();
        float maxApertureSpeed();
    
        /** Tags frames with the generic Lens::Tags defined in the base FCam::Lens class. */
        void tagFrame(FCam::Frame);

        /** The states the lens controller can be in */
        enum LensState {
            NotInitialized,
            NoLens,
            Ready,
            MovingFocus,
            MovingAperture,
            Busy
        };

        /** Read the current state of the lens */
        LensState getState();
        /** Attempt to reinitialize the lens controller */
        void reset();

        // Lens errors
        struct LensError {
            enum e {
                None=0,
                UnrecognizedCommand=1,
                LensIsManualFocus=2,
                NoLensConnected=3,
                LensDistanceStopError=4,
                ApertureNotInitialized=5,
                InvalidBaudRate=6,
                Reserved1=7,
                Reserved2=8,
                BadParameter=9,
                XModemTimeout=10,
                XModemError=11,
                XModemUnlockCodeIncorrect=12,
                NotUsed1=13,
                InvalidPort=14,
                LicenseUnlockFailure=15,
                InvalidLicenseFile=16,
                InvalidLibraryFile=17,
                Reserved3=18,
                Reserved4=19,
                NotUsed2=20,
                LibraryNotReadyForLens=21,
                LibraryNotReadyForCmds=22,
                CommandNotLicensed=23,
                InvalidFocusRange=24,
                DistanceStopsNotSupported=25,
                UnknownError=99
            };
        };
    
    private:
        const std::string tty;
        int serial_fd;

        EF232LensDatabase lensDB;
    
        const EF232LensInfo *currentLens;
    
        // Current lens state
        pthread_mutex_t stateMutex;
        LensState state;
        void setState(LensState newState);

        struct LensParams {
            Time time;
            unsigned int focalLength; // mm
            unsigned int aperture;    // 10x f-number
            unsigned int focusDist;   // encoder counts
        };
        CircularBuffer<LensParams> lensHistory;

        int calcFocusDistance(const Time &t) const;
        int calcAperture(const Time &t) const;
        int calcFocalLength(const Time &t) const;

        // Encoder->diopter conversion
        unsigned int focusEncoderMax;
        float diopScaleFactor;

        float encToDiopter(unsigned int encoder);
        unsigned int diopToEncoder(float diopters);

        enum LensCmd {
            Initialize,
            Calibrate,
            SetAperture,
            SetFocus,
            Shutdown
        };

        struct LensOrder {
            LensCmd cmd;
            unsigned int val;
        };

        TSQueue<LensOrder> cmdQueue;

        // Lens control thread
        pthread_t controlThread;
        bool controlRunning;
        void launchControlThread();
        void runLensControlThread();

        static void *lensControlThread(void *arg);                  

        // High-level lens control functions

        void init();
        void calibrateLens();

        void idleProcessing();

        // Individual command methods

        // Shared buf for parsing
        std::string buf;

        std::string cmd_GetID();
        unsigned int cmd_GetFocalLength(std::string idStr="");
        unsigned int cmd_GetMinAperture(std::string idStr="");

        void cmd_GetZoomRange(unsigned int &min,
                              unsigned int &max);
        void cmd_GetFocusBracket(unsigned int &min,
                                 unsigned int &max);
        unsigned int cmd_GetFocusEncoder();

        void cmd_SetFocusEncoder(unsigned int val);

        LensError::e cmd_DoInitialize();

        unsigned int cmd_DoApertureOpen();
        unsigned int cmd_DoAperture(unsigned int val);
        unsigned int cmd_DoApertureClose();

        void cmd_DoFocusAtZero();
        unsigned int cmd_DoFocus(unsigned int val);
        void cmd_DoFocusAtInf();

   
        // Basic communication functions
        void send(const std::string&);
        template<typename T>
        void read(T &val);
        void expect(const std::string& desired);
        void expect(const std::string& desired, std::string& remainder);
        LensError::e errorCheck(std::string &remainder);
    };

}
}

#endif
