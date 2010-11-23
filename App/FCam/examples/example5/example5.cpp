#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <FCam/N900.h>
#include <FCam/AutoFocus.h>

/** \file */

// Select the platform
namespace Plat = FCam::N900;
// namespace Plat = FCam::F2;

/***********************************************************/
/* Autofocus                                               */
/*                                                         */
/* This example shows how to request streams and deal with */
/* the incoming frames, and also uses the provided         */
/* autofocus routine.                                      */
/***********************************************************/
int main(int argc, char ** argv) {

    // Devices 
    Plat::Sensor sensor;
    Plat::Lens lens;
    sensor.attach(&lens); // Attach the lens to the sensor

    // Autofocus supplied by FCam API 
    FCam::AutoFocus autoFocus(&lens);

    // Shot 
    FCam::Shot stream1;
    // Set the shot parameters
    stream1.exposure = 50000;
    stream1.gain = 1.0f;

    // Request a resolution, and allocate storage
    stream1.image = FCam::Image(640, 480, FCam::UYVY);

    // Enable the sharpness unit
    stream1.sharpness.enabled = true;

    // We will stream until the focus stabilizes
    int count = 0;        // # of frames streamed

    // Order the sensor to stream
    sensor.stream(stream1);

    // Ask the autofocus algorithm to start sweeping the lens
    autoFocus.startSweep();

    // Stream until autofocus algorithm completes
    FCam::Frame frame;

    do {
        // Retrieve a frame
        frame = sensor.getFrame();
        assert(frame.shot().id == stream1.id); // Check the source of the request
      
        // The lens has tagged each frame with where it was focused
        // during that frame. Let's retrieve it so we can print it out.
        float diopters = frame["lens.focus"];
        printf("Lens focused at %2.0f cm\n", 100/diopters);

        // The sensor has attached a sharpness map to each frame. 
        // Let's sum up all the values in it so we can print out 
        // the total sharpness of this frame.
        int totalSharpness = 0;
        for (int y = 0; y < frame.sharpness().height(); y++) {
            for (int x = 0; x < frame.sharpness().width(); x++) {
                totalSharpness += frame.sharpness()(x, y);
            }
        }
        printf("Total sharpness is %d\n\n", totalSharpness);

        // Call the autofocus algorithm
        autoFocus.update(frame);
      
        // Increment frame counter
        count++;
    } while (!autoFocus.idle());

    printf("Autofocus chose to focus at %2.0f cm\n\n", 100/lens.getFocus());

    // Write out the focused frame
    FCam::saveJPEG(frame, "/home/user/MyDocs/DCIM/example5.jpg");

    // Order the sensor to stop streaming
    sensor.stopStreaming();
    printf("Processed %d frames until autofocus completed!\n", count);

    // There may still be shots in the pipeline. Consume them.
    while (sensor.shotsPending() > 0) frame = sensor.getFrame();

    // Check that the pipeline is empty
    assert(sensor.framesPending() == 0);
    assert(sensor.shotsPending() == 0);
}
