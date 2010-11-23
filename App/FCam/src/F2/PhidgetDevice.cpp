#include <stdio.h>

#include "FCam/Frame.h"

#include "PhidgetDevice.h"
#include "../Debug.h"

namespace FCam {
namespace F2 {

std::vector<PhidgetDevice *> PhidgetDevice::inputChangeHandlers[8];
std::vector<PhidgetDevice *> PhidgetDevice::outputChangeHandlers[8];
std::vector<PhidgetDevice *> PhidgetDevice::sensorChangeHandlers[8];
bool PhidgetDevice::phidgetsAvailable;
CPhidgetInterfaceKitHandle PhidgetDevice::ifKit;

  
PhidgetDevice::PhidgetDevice() {
    if (phidgetsAvailable == false) {
        printf("Phidgets aren't ready yet, lets set it up.\n");
        //printf("phidget ifKit is initially %x\n", (unsigned int) ifKit);
        CPhidgetInterfaceKit_create(&ifKit);
        //printf("then it is  %x\n", (unsigned int) ifKit);
        CPhidget_set_OnAttach_Handler((CPhidgetHandle)ifKit, phidgetAttachHandler, this);
        CPhidget_set_OnDetach_Handler((CPhidgetHandle)ifKit, phidgetDetachHandler, this);
        CPhidget_set_OnError_Handler((CPhidgetHandle)ifKit, phidgetErrorHandler, this);

        CPhidgetInterfaceKit_set_OnInputChange_Handler (ifKit, phidgetInputChangeHandler, this);
        CPhidgetInterfaceKit_set_OnSensorChange_Handler (ifKit, phidgetSensorChangeHandler, this);
        CPhidgetInterfaceKit_set_OnOutputChange_Handler (ifKit, phidgetOutputChangeHandler, this);

        CPhidget_open((CPhidgetHandle)ifKit, -1);
        int result;
        const char * errMsg;
        if((result = CPhidget_waitForAttachment((CPhidgetHandle)ifKit, 1000))) {
            CPhidget_getErrorDescription(result, &errMsg);
            printf("Problem waiting for attachment: %s\n", errMsg);
            printf("Continuing without Phidgets support\n");
        } else {
            phidgetsAvailable = true;
            CPhidgetInterfaceKit_setOutputState(ifKit, 7, true);
        }
    }   else {
        printf("Another phidget device already set up phidgets\n");
    } 
}

void tagFrame(FCam::Frame f) {};
  
void PhidgetDevice::registerInputChangeHandler(int index, PhidgetDevice * device) {
    inputChangeHandlers[index].push_back(device);
}

void PhidgetDevice::registerOutputChangeHandler(int index, PhidgetDevice * device) {
    outputChangeHandlers[index].push_back(device);
}

void PhidgetDevice::registerSensorChangeHandler(int index, PhidgetDevice * device) {
    sensorChangeHandlers[index].push_back(device);
}  

int PhidgetDevice::phidgetAttachHandler(CPhidgetHandle IFK, void *userptr) {
    // Maybe this should print some status information? (could also be annoying)
    return 0;
}

int PhidgetDevice::phidgetDetachHandler(CPhidgetHandle IFK, void *userptr) {
    // Status update might be nice here (could also be annoying, ignore for now)
    return 0;
}

int PhidgetDevice::phidgetErrorHandler(CPhidgetHandle IFK, void *userptr, int errCode, const char *errMsg) {    
    printf("Phidgets Error %d: %s\n", errCode, errMsg);
    return 0;
}

int PhidgetDevice::phidgetInputChangeHandler(CPhidgetInterfaceKitHandle IFK, void *userptr, int index, int state) {
    int result = 0;
    // Return the last handler's result (only weird if multiple handlers for a single index)
    if (inputChangeHandlers[index].size() > 0){
        PhidgetDevice * device;
        for (unsigned int i = 0; i < inputChangeHandlers[index].size(); ++i){
            device = inputChangeHandlers[index][i];
            result = device->handleInputChange(IFK, index, state);
            //printf("just called device %x with index %d and state %d\n", (unsigned int) device, index, state);
        }
    } else {
        printf("No handler found: Input %d changed to value %d.\n", index, state);      
    }
    return result;
}

int PhidgetDevice::phidgetOutputChangeHandler(CPhidgetInterfaceKitHandle IFK, void *userptr, int index, int state) {
    int result = 0;
    // Return the last handler's result (only weird if multiple handlers for a single index)
    if (outputChangeHandlers[index].size() > 0){
        PhidgetDevice * device;
        for (unsigned int i = 0; i < outputChangeHandlers[index].size(); ++i){
            device = outputChangeHandlers[index][i];
            result = device->handleOutputChange(IFK, index, state);
        }
    } else {
        printf("No handler found: Output %d changed to value %d.\n", index, state);      
    }
    return result;



}

int PhidgetDevice::phidgetSensorChangeHandler(CPhidgetInterfaceKitHandle IFK, void *userptr, int index, int value) {
    int result = 0;
    // Return the last handler's result (only weird if multiple handlers for a single index)
    if (sensorChangeHandlers[index].size() > 0){
        PhidgetDevice * device;
        for (unsigned int i = 0; i < sensorChangeHandlers[index].size(); ++i){
            device = sensorChangeHandlers[index][i];
            result = device->handleSensorChange(IFK, index, value);
        }
    } else {
        printf("No handler found: Sensor %d changed to value %d.\n", index, value);      
    }
    return result;
}

int PhidgetDevice::handleInputChange(CPhidgetInterfaceKitHandle IFK, int index, int state) {
    printf("Default Handler: Input %d changed to state %d.\n", index, state);
    return 0;
}
int PhidgetDevice::handleOutputChange(CPhidgetInterfaceKitHandle IFK, int index, int state) {
    printf("Default Handler: Output %d changed to state %d.\n", index, state);
    return 0;
}
int PhidgetDevice::handleSensorChange(CPhidgetInterfaceKitHandle IFK, int index, int value) {
    printf("Default Handler: Sensor %d changed to value %d.\n", index, value);
    return 0;
}
}
}


  



