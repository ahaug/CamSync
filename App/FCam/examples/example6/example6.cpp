#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <FCam/N900.h>

#include "SoundPlayer.h"

/** \file */

// Select the platform
namespace Plat = FCam::N900;
// namespace Plat = FCam::F2;

/***********************************************************/
/* Shutter sound                                           */
/*                                                         */
/* This example shows how to declare and attach a device,  */
/* and write the appropriate actions. In this example, the */
/* camera will trigger two actions at the beginning of the */
/* exposure: a flash, and a shutter sound.                 */
/* See SoundPlayer class for more information.             */
/***********************************************************/
int main(int argc, char ** argv) {

    // Devices
    Plat::Sensor sensor;
    Plat::Flash flash;
    
    // We defined a custom device to play a sound during the
    // exposure. See SoundPlayer.h/cpp for details.
    SoundPlayer audio; 
    
    sensor.attach(&flash); // Attach the flash to the sensor
    sensor.attach(&audio); // Attach the sound player to the sensor
    
    // Set the shot parameters
    FCam::Shot shot1;
    shot1.exposure = 400000;
    shot1.gain = 1.0f;
    shot1.image = FCam::Image(2592, 1968, FCam::UYVY);
    
    // Action (Flash)
    FCam::Flash::FireAction fire(&flash);
    fire.time = 0; 
    fire.duration = 60000;
    fire.brightness = flash.maxBrightness();
    
    // Action (Sound)
    SoundPlayer::SoundAction click(&audio);
    click.time = 0; // Start at the beginning of the exposure
    click.setWavFile("/usr/share/sounds/camera_snd_title_1.wav");
    
    // Attach actions
    shot1.addAction(fire);
    shot1.addAction(click);
    
    // Order the sensor to capture a shot.
    // The flash and the shutter sound should happen simultaneously.
    sensor.capture(shot1);
    assert(sensor.shotsPending() == 1); // There should be exactly one shot
    
    // Retrieve the frame from the sensor
    FCam::Frame frame = sensor.getFrame();
    assert(frame.shot().id == shot1.id); // Check the source of the request
    
    // Write out the file
    FCam::saveJPEG(frame, "/home/user/MyDocs/DCIM/example6.jpg");
    
    // Check that the pipeline is empty
    assert(sensor.framesPending() == 0);
    assert(sensor.shotsPending() == 0);
}
