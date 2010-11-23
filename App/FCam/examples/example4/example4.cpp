#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <FCam/N900.h>
#include <FCam/AutoExposure.h>
#include <FCam/AutoWhiteBalance.h>

/** \file */

// Select the platform
namespace Plat = FCam::N900;
// namespace Plat = FCam::F2;

/***********************************************************/
/* Autoexposure                                            */
/*                                                         */
/* This example shows how to request streams and deal with */
/* the incoming frames, and also uses the provided         */
/* auto-exposure and auto-white-balance routines.          */
/***********************************************************/
int main(int argc, char ** argv) {

    // Make a sensor
    Plat::Sensor sensor;
    
    // Shot
    FCam::Shot stream1;
    // Set the shot parameters
    stream1.exposure = 33333;
    stream1.gain = 1.0f;

    // We don't bother to set frameTime in this example. It defaults
    // to zero, which the implementation will clamp to the minimum
    // possible value given the exposure time.

    // Request an image size and allocate storage
    stream1.image = FCam::Image(640, 480, FCam::UYVY);

    // Enable the histogram unit
    stream1.histogram.enabled = true;
    stream1.histogram.region = FCam::Rect(0, 0, 640, 480);
    
    // We will stream until the exposure stabilizes
    int count = 0;          // # of frames streamed
    int stableCount = 0;    // # of consecutive frames with stable exposure
    float exposure;         // total exposure for the current frame (exposure time * gain)
    float lastExposure = 0; // total exposure for the previous frame 
    
    FCam::Frame frame;

    do {
        // Ask the sensor to stream with the given parameters
        sensor.stream(stream1);
    
        // Retrieve a frame
        frame = sensor.getFrame();
        assert(frame.shot().id == stream1.id); // Check the source of the request

        printf("Exposure time: %d, gain: %f\n", frame.exposure(), frame.gain());

        // Calculate the total exposure used (including gain)
        exposure = frame.exposure() * frame.gain();
    
        // Call the autoexposure algorithm. It will update stream1
        // using this frame's histogram.
        autoExpose(&stream1, frame);
    
        // Call the auto white-balance algorithm. It will similarly
        // update the white balance using the histogram.
        autoWhiteBalance(&stream1, frame);

        // Increment stableCount if the exposure is within 5% of the
        // previous one
        if (fabs(exposure - lastExposure) < 0.05f * lastExposure) {
            stableCount++;
        } else {
            stableCount = 0;
        }

        // Update lastExposure
        lastExposure = exposure;
    
    } while (stableCount < 5); // Terminate when stable for 5 frames

    // Write out the well-exposed frame
    FCam::saveJPEG(frame, "/home/user/MyDocs/DCIM/example4.jpg");
    
    // Order the sensor to stop streaming
    sensor.stopStreaming();
    printf("Processed %d frames until stable for 5 frames!\n", count);
    
    // There may still be shots in the pipeline. Consume them.
    while (sensor.shotsPending() > 0) frame = sensor.getFrame();

    // Check that the pipeline is empty
    assert(sensor.framesPending() == 0);
    assert(sensor.shotsPending() == 0);
}
