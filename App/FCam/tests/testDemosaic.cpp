#include "FCam/FCam.h"
#include <stdio.h>
#include <math.h>

using namespace FCam;

Shot _shot;

float _colorMatrix[12] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0};

const std::string _string = "Test";

class TestFrame : public _Frame {
public:
    const FCam::Shot &baseShot() const {return _shot;}
    FCam::BayerPattern bayerPattern() const {return FCam::GRBG;}
    unsigned short maxRawValue() const {return 1023;}
    void rawToRGBColorMatrix(int, float *m) const {
        for (int i = 0; i < 12; i++) m[i] = _colorMatrix[i];
    }
    const std::string &manufacturer() const {return _string;}
    const std::string &model() const {return _string;}
};

int main(int argc, const char **argv) {
    
    TestFrame *_f = new TestFrame;
    Frame f(_f);
    
    printf("Testing demosaic results on a white jaggy circle\n");
    Image circle(128, 128, RAW);
    short *data = (short *)circle(0,0);
    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            bool in = ((x-64)*(x-64) + (y-64)*(y-64)) < 2500;
            *data++ = in ? 1023 : 0;
        }
    }
    
    _f->image = circle;
    FCAM_IMAGE_DEBUG(f.image());
    saveDump(demosaic(f), "circle.tmp");

    printf("Testing demosaic results on a white anti-aliased circle\n");
    data = (short *)circle(0,0);
    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            float d = sqrtf(((x-64)*(x-64) + (y-64)*(y-64)));            
            *data++ = (d > 50 ? 0 : (d < 49 ? 1023 : (50 - d)*1023));
        }
    }

    saveDump(circle, "circle_in.tmp");
    saveDump(demosaic(f), "circle_out.tmp");

    printf("Testing demosaic on a subimage\n");
    Image canvas(200,200, RAW);
    for (int y=0; y < 200; y++) {
        for (int x=0; x < 200; x+=2) {
            ((short*)canvas(x,y))[0] = 0;
            ((short*)canvas(x,y))[1] = 1023;
        }
    }
    Image subCanvas = canvas.subImage(36,36,Size(128,128));    
    subCanvas.copyFrom(circle);
    FCAM_IMAGE_DEBUG(canvas);
    FCAM_IMAGE_DEBUG(subCanvas);
    saveDump(canvas, "circle_canvas_in.tmp");
    saveDump(subCanvas, "circle_sub_in.tmp");
    saveDump(demosaic(f), "circle_sub.tmp");

    printf("Testing demosaic speed\n");
    
    Image in(64*40, 48*40, RAW);

    _f->image = in;

    Time t1 = Time::now();
    
    for (int i = 0; i < 4; i++) {
        demosaic(f);
    }
    
    printf("%d\n", (Time::now() - t1)/4000);

    printf("Testing basic thumbnail generation \n");

    Image in2(2592,1968, RAW);
    for (unsigned int y=0; y < in2.height()-1; y+=2) {
        for (unsigned int x=0; x < in2.width()-1; x+=2) {
            *((short*)in2(x,y))   = 500 * ((x / 50) % 2) * ((y / 50) % 2); // G
            *((short*)in2(x+1,y)) = 500 * ((x / 25) % 2) * ((y / 25) % 2); // R
            *((short*)in2(x,y+1)) = 500 * ((x / 100) % 2) * ((y / 100) % 2); // B
            *((short*)in2(x+1,y+1))   = 500 * ((x / 50) % 2) * ((y / 50) % 2); // G
        }
    }
    saveDump(in2, "thumb_in.tmp");
    _f->image = in2;
    saveDump(makeThumbnail(f), "thumb_out.tmp");

    #ifdef FCAM_ARCH_ARM
    printf("Testing thumbnail speed, N900-asm 2592x1968 GRBG -> 640x480 \n");
    
    _f->image = in2;
    Time t3 = Time::now();
    for (int i=0; i<10; i++) {
        makeThumbnail(f);
    }
    printf("%d\n", (Time::now() - t3)/10000);
    #endif

    printf("Testing thumbnail speed, generic 2591x1967 GRBG -> 640x480 \n");
    Image in3(2591,1967, RAW);
    _f->image = in3;
    Time t2 = Time::now();
    for (int i=0; i<10; i++) {
        makeThumbnail(f);
    }
    printf("%d\n", (Time::now() - t2)/10000);


    return 0;
};
