#include <FCam/N900.h>
#include <iostream>

using namespace FCam;

// Test streaming a full resolution, raw, at 10fps

int main(int argc, char **argv) {
    N900::Sensor sensor;

    Shot shot;
    shot.image = Image(sensor.maxImageSize(), RAW, Image::AutoAllocate);
    shot.exposure = 1000;
    shot.gain = 1.0f;
    shot.frameTime = 100000;

    sensor.stream(shot);
    
    for (int i = 0; i < 100; i++) {
        Frame f = sensor.getFrame();
        printf("%d: %x\n", i, f.image()(0, 0));

        Event e;
        while (getNextEvent(&e)) {            
            std::cout << "Event: " << e.description << std::endl;
        }
    }

    return 0;
}
