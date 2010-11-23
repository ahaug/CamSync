#include <FCam/N900.h>
#include <stdio.h>
#include <iostream>

using namespace FCam;

// to use this, place the N900 face down on a white piece of paper
// with the lens cap open. It will use the rolling shutter to
// calculate flash latency (and rolling shutter speed).

int main(int argc, char **argv) {
    printf("Constructing sensor\n");
    N900::Sensor sensor;
    printf("Constructing flash\n");
    N900::Flash flash;
    printf("Attaching flash to sensor\n");
    sensor.attach(&flash);
    printf("Constructing shot\n");
    Shot shot;
    shot.exposure = 100;
    shot.gain = 8.0f;
    shot.frameTime = 200000;
    shot.image = Image(2592, 1968, RAW, Image::AutoAllocate);

    printf("Constructing fire action\n");
    Flash::FireAction fire(&flash);
    fire.duration = flash.minDuration();
    fire.brightness = flash.maxBrightness();
    shot.actions.insert(&fire);

    printf("Capturing shot\n");
    int time[80];
    int scanline[80];
    for (int i = 0; i < 80; i++) {
        time[i] = i*500 - 500;
        fire.time = time[i];
        sensor.capture(shot);
    }

    printf("Analyzing images\n");
    for (int i = 0; i < 80; i++) {        
        scanline[i] = -1;
        Frame f = sensor.getFrame();
        Image im = f.image();

        Flash::Tags tags(f);

        std::cout << "Flash tags: " 
                  << tags.start << ", " 
                  << tags.duration  << ", "
                  << tags.brightness << ", "
                  << tags.peak << std::endl;
        if (!im.valid()) {
            printf("Image is not valid!\n");
        } else {
            for (int y = 0; y < im.height(); y++) {
                int sum = 0;
                for (int x = 0; x < 128; x++) {
                    unsigned b = ((unsigned short *)(im(x, y)))[0];
                    sum += b;
                }
                if (sum > 1000) {
                    scanline[i] = y;
                    break;
                }
            }
        }
        printf("%d: %d\n", time[i] + f.exposure(), scanline[i]);
    }
    
    // make a line of best fit through all the good data
    double sx2 = 0;
    double sx = 0;
    double sy = 0;
    double sxy = 0;
    int n = 0;
    for (int i = 0; i < 80; i++) {
        if (scanline[i] <= 100) continue;
        n++;
        sx2 += scanline[i]*scanline[i];
        sx += scanline[i];
        sxy += time[i]*scanline[i];
        sy += time[i];
    }

    double invDet = 1.0/(sx2*n - sx*sx);
    double a = invDet * (n * sxy - sx*sy);
    double b = invDet * (sx2*sy - sx*sxy);
    double shutterTime = shot.image.height()*a;
    printf("Rolling shutter time is %f us\n", shutterTime);
    printf("Flash latency is %f us\n", b);
   

    return 0;
}

