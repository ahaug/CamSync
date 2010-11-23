#include <FCam/N900.h>

// This program tests repeatedly restarting the sensor

using namespace FCam;

int main(int, char **) {

    for (int i = 0; i < 10; i++) {
        printf("Firing up sensor...\n");
        N900::Sensor sensor;
        Shot shot;
        shot.exposure = 30000;
        shot.gain = 2.0f;
        shot.image = FCam::Image(640, 480, UYVY);
        sensor.stream(shot);
        Frame frame = sensor.getFrame();
        char buf[128];
        sprintf(buf, "test_%d.jpg", i);
        saveJPEG(frame, buf, 50);
        printf("Shutting down sensor...\n");
    }
}
