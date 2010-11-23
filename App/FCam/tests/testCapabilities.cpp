#include "FCam/N900.h"
#include <vector>
#include <iostream>

using namespace FCam;

int main(int argc, char **argv) {
    printf("Testing the limits of the N900 sensor. Please leave the lens cover"
           " closed for this test.\n");
    
    N900::Sensor sensor;
    Shot shot;
    Frame frame;
    
    
    std::vector<Image> targets;
    targets.push_back(Image(sensor.minImageSize(), UYVY, Image::AutoAllocate));
    targets.push_back(Image(sensor.minImageSize(), RAW, Image::AutoAllocate));
    targets.push_back(Image(320, 240, UYVY, Image::AutoAllocate));
    targets.push_back(Image(320, 240, RAW, Image::AutoAllocate));
    targets.push_back(Image(640, 480, UYVY, Image::AutoAllocate));
    targets.push_back(Image(640, 480, RAW, Image::AutoAllocate));
    targets.push_back(Image(800, 600, UYVY, Image::AutoAllocate));
    targets.push_back(Image(800, 600, RAW, Image::AutoAllocate));
    targets.push_back(Image(1280, 960, UYVY, Image::AutoAllocate));
    targets.push_back(Image(1280, 960, RAW, Image::AutoAllocate));
    targets.push_back(Image(2560, 1920, UYVY, Image::AutoAllocate));
    targets.push_back(Image(2560, 1920, RAW, Image::AutoAllocate));
    targets.push_back(Image(sensor.maxImageSize(), UYVY, Image::AutoAllocate));
    targets.push_back(Image(sensor.maxImageSize(), RAW, Image::AutoAllocate));
    for (int i = 0; i < targets.size(); i++) {
        Frame f1;
        shot.exposure = 30000000;
        shot.frameTime = 60000000;
        shot.gain = 1.0f;
        shot.image = targets[i];
        sensor.capture(shot);

        Frame f2;
        shot.exposure = 0;
        shot.frameTime = 0;
        shot.gain = 1.0f;
        shot.image = targets[i];
        sensor.capture(shot);

        Event e;
        while (getNextEvent(&e)) {
            std::cout << e.description << std::endl;
        }

        f1 = sensor.getFrame();
        f2 = sensor.getFrame();

        if (!f1.image().valid() || !f2.image().valid()) {
            printf("INVALID IMAGE\n");
        } else {            
            printf("%dx%d -> %dx%d %s -> Exposure: [%d %d], frame time: [%d %d]\n", 
                   targets[i].width(), targets[i].height(),
                   f1.image().width(), f1.image().height(), 
                   f1.image().type() == RAW ? "raw" : "uyvy",
                   f2.exposure(), f1.exposure(), f2.frameTime(), f1.frameTime());
        }
    }
    

    // Check which resolutions are good for UYVY (close the shutter,
    // and inspect the output images to see if the noise pattern
    // displays significant banding indicating a slight resample)
    shot.exposure = 200000;
    shot.gain = 32;

    printf("Using exposure = %d, gain = %f\n", shot.exposure, shot.gain);

    int widths[] = {2560, 2576, 2592};
    int heights[] = {1954, 1954, 1954};
    for (int i = 0; i < sizeof(widths)/sizeof(widths[0]); i++) {
        int w = widths[i];
        int h = heights[i];
        printf("%d %d\n", w, h);
        shot.image = Image(w, h, UYVY);
        sensor.capture(shot);
        Frame f = sensor.getFrame();
        char buf[128];
        sprintf(buf, "test_%d_%d.jpg", w, h);
        saveJPEG(f, buf, 100);
    }
    
    return 0;
    

}
