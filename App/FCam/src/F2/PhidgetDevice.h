#ifndef FCAM_PHIDGETDEVICE_H
#define FCAM_PHIDGETDEVICE_H

/** \file
 * This file contains a base class for devices that represent a physical device connected over Phidgets.
 * 
 */
#include "FCam/Device.h"

#include "linux/phidget21.h"
#include <vector>
#include <utility>

namespace FCam {
namespace F2 {
/*! A manager and base class for Phidgets based devices.
 * This class provides convenience methods for attaching Phidget interface kits
 * to the running process and registering handler functions for Phidget events.
 * In order to register a device as a Phidget event handler, it must be a
 * PhidgetDevice or one of its subclasses. Note that this class itself does not define any actions,
 * and cannot tag frames.
 */
class PhidgetDevice : public FCam::Device {
public:
    /*! The constructor for PhidgetDevices automatically searches for and attaches 
     * the Phidget interface kit if a previous device has not already done so.
     * Subclasses of PhidgetDevice must call the super constructor to maintain this
     * functionality.
     */
    PhidgetDevice();
    //! PhidgetDevices subclasses are free to override this function to tag frames like other devices.
    virtual void tagFrame(Frame);
    
protected:
    //! This is the shared interface kit handle for all Phidget devices.
    //! Subclasses of PhidgetDevice can use this handle for other Phidget actions, such as changing an output value.
    static CPhidgetInterfaceKitHandle ifKit;
    //! Set to true when a Phidget interface kit is successfully attached.
    static bool phidgetsAvailable;
    //! Use this method to register a PhidgetDevice to receive input change Phidget events.
    static void registerInputChangeHandler(int index, PhidgetDevice * device);
    //! Use this method to register a PhidgetDevice to receive output change Phidget events.
    static void registerOutputChangeHandler(int index, PhidgetDevice * device);
    //! Use this method to register a PhidgetDevice to receive sensor (analog input) change Phidget events.
    static void registerSensorChangeHandler(int index, PhidgetDevice * device);
    
    
    //! This method gets called by the Phidget event dispatcher if this PhidgetDevice has registered 
    //! to recieve input change events for this input index. Subclasses should override this method
    //! to perform custom behavior in response to events.
    virtual int handleInputChange(CPhidgetInterfaceKitHandle IFK, int index, int state);
    //! This method gets called by the Phidget event dispatcher if this PhidgetDevice has registered 
    //! to recieve output change events for this output index. Subclasses should override this method
    //! to perform custom behavior in response to events.
    virtual int handleOutputChange(CPhidgetInterfaceKitHandle IFK, int index, int state);
    //! This method gets called by the Phidget event dispatcher if this PhidgetDevice has registered 
    //! to recieve sensor change events for this sensor index. Subclasses should override this method
    //! to perform custom behavior in response to events.
    virtual int handleSensorChange(CPhidgetInterfaceKitHandle IFK, int index, int state);
private:
    static std::vector<PhidgetDevice *> inputChangeHandlers[8];
    static std::vector<PhidgetDevice *> outputChangeHandlers[8];
    static std::vector<PhidgetDevice *> sensorChangeHandlers[8];
    
    static int phidgetAttachHandler(CPhidgetHandle IFK, void *userptr);
    static int phidgetDetachHandler(CPhidgetHandle IFK, void *userptr);
    static int phidgetErrorHandler(CPhidgetHandle IFK, void *userptr, int errCode, const char *errMsg);
    static int phidgetInputChangeHandler(CPhidgetInterfaceKitHandle IFK, void *userptr, int index, int state);
    static int phidgetOutputChangeHandler(CPhidgetInterfaceKitHandle IFK, void *userptr, int index, int state);
    static int phidgetSensorChangeHandler(CPhidgetInterfaceKitHandle IFK, void *userptr, int index, int value);    
};

}
}


#endif
