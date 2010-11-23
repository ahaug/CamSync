#include "FCam/F2/ShutterButton.h"

#include "PhidgetDevice.h"
#include "../Debug.h"

namespace FCam { namespace F2 {

    class PhidgetShutterButton: public PhidgetDevice {

    public:
        PhidgetShutterButton(int halfPress, int fullPress): 
            PhidgetDevice(),
            halfPressIndex(halfPress),
            fullPressIndex(fullPress)
        {

            PhidgetDevice::registerInputChangeHandler(halfPressIndex, this);
            PhidgetDevice::registerInputChangeHandler(fullPressIndex, this);    
        }

    protected:

        //! The handler function for half presses. This simply generates and enqueues a FocusPressed or FocusReleased type Event.
        int halfPressHandler(CPhidgetInterfaceKitHandle interfaceKit, int index, int state);
        //! The handler function for half presses. This simply generates and enqueues a ShutterPressed or ShutterReleased type Event.
        int fullPressHandler(CPhidgetInterfaceKitHandle interfaceKit, int index, int state);
        //! An overridden implementation of the basic PhidgetDevice input change handler. It simply hands off the event to 
        //! the appropriate half or full press handler depending on the index of the PhidgetEvent.
        virtual int handleInputChange(CPhidgetInterfaceKitHandle interfaceKit, int index, int state);

    private:
        //! The Phidget input index for a half shutter press.
        int halfPressIndex; // default indices in the phidgets kit
        //! The Phidget input index for a full shutter press.
        int fullPressIndex;    

    };

    int PhidgetShutterButton::handleInputChange(CPhidgetInterfaceKitHandle interfaceKit, int index, int state){
        printf("handle input change for shutter button called.\n");
        if (index == halfPressIndex) {
            return halfPressHandler(interfaceKit, index, state);
        }
        if (index == fullPressIndex) {
            return fullPressHandler(interfaceKit, index, state);
        }
        return 0;
    }
    
    int PhidgetShutterButton::halfPressHandler(CPhidgetInterfaceKitHandle interfaceKit, int index, int state){
        if (index == halfPressIndex) {
            Event event;
            event.time = Time::now();
            event.data = 0;
            event.creator = this;
            switch(state) {
            case true:
                event.type = Event::FocusPressed;     
                break;
            case false:
            default:
                event.type = Event::FocusReleased;
                break;        
            }
            postEvent(event);
        }
        return 0;
    }
    int PhidgetShutterButton::fullPressHandler(CPhidgetInterfaceKitHandle interfaceKit, int index, int state){
        if (index == fullPressIndex) {
            Event event;
            event.time = Time::now();
            event.data = 0;
            event.creator = this;
            switch(state) {
            case true:
                event.type = Event::ShutterPressed;   
                break;
            case false:
            default:
                event.type = Event::ShutterReleased;
                break;        
            }
            printf("Posting full press\n");
            postEvent(event);
        }
        return 0;
    }
  
    ShutterButton::ShutterButton(int halfPress, int fullPress): impl(NULL) {
        impl = new PhidgetShutterButton(halfPress,fullPress);

    }

    ShutterButton::~ShutterButton() {
        if (impl) delete impl;
    }

    
    std::string ShutterButton::getEventString(const Event & event) const {    
        switch (event.type) {
        case Event::FocusPressed:
            return std::string("Shutter button half-depressed.");
            break;
        case Event::FocusReleased:
            return std::string("Shutter button fully released.");
            break;
        case Event::ShutterPressed:
            return std::string("Shutter button fully depressed.");
            break;
        case Event::ShutterReleased:
            return std::string("Shutter button half-released.");
            break;
        default:
            return std::string("Unknown event type for ShutterButton device.");
            break;
        
        }
    }
}}

