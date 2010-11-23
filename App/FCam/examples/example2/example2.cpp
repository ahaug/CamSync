#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <FCam/N900.h>

/** \file */

// Select the platform
namespace Plat = FCam::N900;
// namespace Plat = FCam::F2;

/***********************************************************/
/* Flash / No-flash                                        */
/*                                                         */
/* This example demonstrates capturing multiple shots with */
/* possibly different settings.                            */
/***********************************************************/

int main(int argc, char ** argv) {

    // Devices
    Plat::Sensor sensor;
    Plat::Flash flash;
    sensor.attach(&flash); // Attach the flash to the sensor
    
    
    // Make two shots
    std::vector<FCam::Shot> shot(2);
    
    // Set the first shot parameters (to be done with flash)
    shot[0].exposure = 80000;
    shot[0].gain = 1.0f;
    shot[0].image = FCam::Image(2592, 1968, FCam::UYVY);
    
    // Set the second shot parameters (to be done without flash)
    shot[1].exposure = 80000;
    shot[1].gain = 1.0f;
    shot[1].image = FCam::Image(2592, 1968, FCam::UYVY);
    
    // Make an action to fire the flash
    Plat::Flash::FireAction fire(&flash);
    fire.duration = flash.minDuration();          // flash briefly
    fire.time = shot[0].exposure - fire.duration; // at the end of the exposure
    fire.brightness = flash.maxBrightness();      // at full power
    
    // Attach the action to the first shot
    shot[0].addAction(fire);
    
    // Order the sensor to capture the two shots
    sensor.capture(shot);
    assert(sensor.shotsPending() == 2);    // There should be exactly two shots
    
    // Retrieve the first frame
    FCam::Frame frame = sensor.getFrame();
    assert(sensor.shotsPending() == 1);    // There should be one shot pending
    assert(frame.shot().id == shot[0].id); // Check the source of the request
    
    // Write out file if needed
    if (argc > 1) FCam::saveJPEG(frame, argv[1]);    
    
    // Retrieve the second frame
    frame = sensor.getFrame();
    assert(frame.shot().id == shot[1].id); // Check the source of the request
    
    // Write out file
    FCam::saveJPEG(frame, "/home/user/MyDocs/DCIM/example2.jpg"); 
    
    // Check the pipeline is empty
    assert(sensor.framesPending() == 0);
    assert(sensor.shotsPending() == 0);
}
