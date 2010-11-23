// TestJPEG.cpp - Simple JPEG saving code test

#include <stdio.h>
#include <FCam/Dummy.h>

int main(int argc, char **argv) {
    
    FCam::Dummy::Sensor sensor;

    FCam::Dummy::Shot shot;
    shot.testPattern = FCam::Dummy::CHECKERBOARD;
    shot.exposure = 5000;
    shot.gain = 1.0f;
    shot.image = FCam::Image(sensor.maxImageSize(), FCam::RGB24);

    sensor.capture(shot);
    FCam::Dummy::Frame frame = sensor.getFrame();

    std::string testName("testJPG_1.jpg");

    saveJPEG(frame, testName);

    FCam::Event e;
    bool errors = false;
    if (FCam::getNextEvent(&e, FCam::Event::Error)) {
        do {
            if (e.type == FCam::Event::Error) {
                errors = true;
                printf("** FCam error [%d] %d at %s: %s\n", e.type, e.data, e.time.toString().c_str(), e.description.c_str());
            }
        } while (FCam::getNextEvent(&e, FCam::Event::Error));
        if (errors) {
            printf ("Error during JPEG testing\n");
            return 1;
        }
    }

    

}
