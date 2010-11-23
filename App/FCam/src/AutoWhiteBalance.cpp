#include "FCam/AutoWhiteBalance.h"
#include "FCam/Sensor.h"
#include "FCam/Platform.h"
#include "Debug.h"


namespace FCam {

void autoWhiteBalance(Shot *s, const Frame &f, 
                      int minWB,
                      int maxWB,
                      float smoothness) {
    if (!s) return;

    if (!f.histogram().valid()) return;

    // auto-white-balance based on the histogram
    int buckets = f.histogram().buckets();
    
    // Compute the mean brightness in each color channel

    int rawRGB[] = {0, 0, 0};

    for (int b = 0; b < buckets; b++) {
        // Assume the color channels are GRBG (should really switch
        // based on the sensor instead)
        rawRGB[0] += f.histogram()(b, 0)*b;
        rawRGB[1] += f.histogram()(b, 1)*b;
        rawRGB[2] += f.histogram()(b, 2)*b;
    }

    // Solve for the linear interpolation between the RAW to sRGB
    // color matrices that makes red = blue. That is, we make the gray
    // world assumption.

    float RGB3200[] = {0, 0, 0};
    float RGB7000[] = {0, 0, 0};
    float d3200[12];
    float d7000[12];
    f.platform().rawToRGBColorMatrix(3200, d3200);
    f.platform().rawToRGBColorMatrix(7000, d7000);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            RGB3200[i] += d3200[i*4+j]*rawRGB[j];
            RGB7000[i] += d7000[i*4+j]*rawRGB[j];
        }
    }

    float alpha = (RGB3200[2] - RGB3200[0])/(RGB7000[0] - RGB3200[0] + RGB3200[2] - RGB7000[2]);

    // inverse wb is used as the interpolant, so there's lots of 1./
    // in this formula to make the interpolant equal alpha as desired.
    int wb = int(1./(alpha * (1./7000-1./3200) + 1./3200));

    if (wb < minWB) wb = minWB;
    if (wb > maxWB) wb = maxWB;

    s->whiteBalance = smoothness * s->whiteBalance + (1-smoothness) * wb;   
}


}
