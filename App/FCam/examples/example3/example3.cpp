#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <FCam/N900.h>

/** \file */

// Select the platform
namespace Plat = FCam::N900;
// namespace Plat = FCam::F2;

/***********************************************************/
/* Focus sweep                                             */
/*                                                         */
/* This example demonstrates moving the lens during an     */
/* exposure via the use of Lens::FocusAction. It also      */
/* shows how to use the metadata tagged by the devices.    */
/* Because the lens on the N900 zooms slightly when it     */
/* focuses, you'll also get a zoom-blur effect.            */
/***********************************************************/
int main(int argc, char ** argv) {

    // Devices
    Plat::Sensor sensor;
    Plat::Lens lens;

    // Attach the lens to the sensor
    sensor.attach(&lens); 
    
    // First focus near with maximal speed
    lens.setFocus(lens.nearFocus(), lens.maxFocusSpeed());
    while (lens.focusChanging()); // Wait to be done
    

    // Now make a shot that will sweep the lens
    FCam::Shot shot1;
    
    FCam::Lens::FocusAction sweep(&lens);
    // Set the parameters of this action
    sweep.time = 0;
    sweep.focus = lens.farFocus();
    sweep.speed = lens.maxFocusSpeed()/4;
    // Calculate how long it takes to move the lens to the desired
    // location in microseconds
    float duration = 1000000.0f *
        (lens.nearFocus() - lens.farFocus()) / sweep.speed;

    printf("The lens will sweep from near to far in %f milliseconds\n",
           duration / 1000.0);

    // Set the shot parameter accordingly
    shot1.exposure = duration;
    shot1.gain = 1.0f;
    // Use a lower resolution to minimize rolling shutter effects
    shot1.image = FCam::Image(640, 480, FCam::UYVY);

    // Attach the action to the shot
    shot1.addAction(sweep);
    
    // Order the sensor to capture the shot
    sensor.capture(shot1);
    assert(sensor.shotsPending() == 1); // There should be exactly one shot
    
    // Retrieve the frame
    FCam::Frame frame = sensor.getFrame();
    assert(frame.shot().id == shot1.id); // Check the source of the request
   
    // Print out some metadata
    const FCam::Lens::Tags lensTags(frame);
    printf("Aperture        : %.4f\n", lensTags.aperture);
    printf("Initial focus   : %.4f\n", lensTags.initialFocus);
    printf("Final focus     : %.4f\n", lensTags.finalFocus);   
    
    // Save the resulting file
    FCam::saveJPEG(frame, "/home/user/MyDocs/DCIM/example3.jpg");

    assert(sensor.framesPending() == 0);
    assert(sensor.shotsPending() == 0);
}
