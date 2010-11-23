#include "FCam/Frame.h"
#include "FCam/F2/Flash.h"

#include "PhidgetDevice.h"
#include "../Debug.h"

namespace FCam { namespace F2 {

    /*! A helper class for the F2::Flash class that handles Phidget communications. 
     */
    class PhidgetFlash : public FCam::F2::PhidgetDevice {
    public:
        //! Instantaneously triggers a flash unit on a given Phidget output index.
        bool triggerFlash(int flashOutputIndex) {
            if (PhidgetDevice::phidgetsAvailable) {
                CPhidgetInterfaceKit_setOutputState(PhidgetDevice::ifKit, flashOutputIndex, true);
                CPhidgetInterfaceKit_setOutputState(PhidgetDevice::ifKit, flashOutputIndex, false);
                return true;
            } else {
                printf("Phidgets aren't available. The flash will not be fired.\n");
                return false;
            }
        }
        //! Begins a flash strobe sequence on a given Phidget output index.
        bool startStrobe(int flashOutputIndex) {
            if (PhidgetDevice::phidgetsAvailable) {
                CPhidgetInterfaceKit_setOutputState(PhidgetDevice::ifKit, flashOutputIndex, true);
                return true;
            } else {
                printf("Phidgets aren't available. The flash will not be fired.\n");
                return false;
            }
        }
        //! Halts a flash strobe sequence on a given Phidget output index.
        bool stopStrobe(int flashOutputIndex) {
            if (PhidgetDevice::phidgetsAvailable) {
                CPhidgetInterfaceKit_setOutputState(PhidgetDevice::ifKit, flashOutputIndex, false);
                return true;
            } else {
                printf("Phidgets aren't available. The flash will not be fired.\n");
                return false;
            }
        }
    };
        

    Flash::Flash(int phidgetOutputIndex): phidgetFlash(NULL){
        phidgetFlash = new PhidgetFlash;
        phidgetIndex = phidgetOutputIndex;
        latencyGuess = 127*1000;
        //phidgetFlash->flashOutputIndex = phidgetIndex;
        //printf("phidgetFlash outputIndex is %d\n", phidgetFlash->flashOutputIndex);
    }
    Flash::~Flash() {
        if (phidgetFlash) delete phidgetFlash;
    }
    
    void Flash::setBrightness(float b) {
        // Yeah, nothing here.
    }
    
    
    void Flash::setDuration(int d) {
        // Figure this out later, probably using something more complicated than phidgets
    }
    
    void Flash::fire(float brightness, int duration) {
        printf("Flash::fire() called, using index %d\n", phidgetIndex);
        phidgetFlash->triggerFlash(phidgetIndex);
    }

    void Flash::startStrobe() {
        phidgetFlash->startStrobe(phidgetIndex);
    }
    void Flash::stopStrobe() {
        phidgetFlash->stopStrobe(phidgetIndex);
    }

    void Flash::tagFrame(FCam::Frame f) {};

    Flash::StrobeStartAction::StrobeStartAction(Flash *f) : flash(f){
        latency = flash->fireLatency();
        time = 0;
    }
    Flash::StrobeStartAction::StrobeStartAction(Flash *f, int t) : flash(f) {
        latency = flash->fireLatency();
        time = t;
    }

    Flash::StrobeStopAction::StrobeStopAction(Flash *f) : flash(f){
        latency = flash->fireLatency();
        time = 0;
    }
    Flash::StrobeStopAction::StrobeStopAction(Flash *f, int t) : flash(f) {
        latency = flash->fireLatency();
        time = t;
    }
  
    void Flash::StrobeStartAction::doAction() {
        flash->startStrobe();
    }
    void Flash::StrobeStopAction::doAction() {      
        flash->stopStrobe();
    }

   
    }
}
