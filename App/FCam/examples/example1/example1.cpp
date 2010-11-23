#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// Select the platform
#include <FCam/N900.h>
namespace Plat = FCam::N900;

/** \file */

/***********************************************************/
/* A program that takes a single shot                      */
/*                                                         */
/* This example is a simple demonstration of the usage of  */
/* the FCam API.                                           */
/***********************************************************/

void errorCheck();

int main(int argc, char ** argv) {

    // Make the image sensor
    Plat::Sensor sensor;
    
    // Make a new shot
    FCam::Shot shot1;
    shot1.exposure = 50000; // 50 ms exposure
    shot1.gain = 1.0f;      // minimum ISO

    // Specify the output resolution and format, and allocate storage for the resulting image
    shot1.image = FCam::Image(2592, 1968, FCam::UYVY);
    
    // Order the sensor to capture a shot
    sensor.capture(shot1);

    // Check for errors 
    errorCheck();

    assert(sensor.shotsPending() == 1); // There should be exactly one shot
    
    // Retrieve the frame from the sensor
    FCam::Frame frame = sensor.getFrame();

    // This frame should be the result of the shot we made
    assert(frame.shot().id == shot1.id);

    // This frame should be valid too
    assert(frame.valid());
    assert(frame.image().valid());
    
    // Save the frame
    FCam::saveJPEG(frame, "/home/user/MyDocs/DCIM/example1.jpg"); 
    
    // Check that the pipeline is empty
    assert(sensor.framesPending() == 0);
    assert(sensor.shotsPending() == 0);

    return 0;
}




void errorCheck() {
    // Make sure FCam is running properly by looking for DriverError
    FCam::Event e;
    while (FCam::getNextEvent(&e, FCam::Event::Error) ) {
        printf("Error: %s\n", e.description.c_str());
        if (e.data == FCam::Event::DriverMissingError) {
            printf("example1: FCam can't find its driver. Did you install "
                   "fcam-drivers on your platform, and reboot the device "
                   "after installation?\n");
            exit(1);
        }
        if (e.data == FCam::Event::DriverLockedError) {
            printf("example1: Another FCam program appears to be running "
                   "already. Only one can run at a time.\n");
            exit(1);
        }        
    }
}
