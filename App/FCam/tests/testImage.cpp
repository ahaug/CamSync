#include "FCam/FCam.h"

#include "../src/Debug.h"
#include <stdio.h>

using namespace FCam;

int main(int argc, const char **argv) {
    printf("Testing the image class\n");
    
    printf("Constructing some images\n");
    Image small(640, 480, UYVY);
    FCAM_IMAGE_DEBUG(small);

    Image big(1024, 1024, UYVY);
    FCAM_IMAGE_DEBUG(big);

    unsigned char data[100*100*2];
    Image weak(100, 100, UYVY, &data[0]);
    FCAM_IMAGE_DEBUG(weak);

    {
        printf("\nTesting copy constructor\n");
        Image newsmall(small);
        FCAM_IMAGE_DEBUG(small);
        FCAM_IMAGE_DEBUG(newsmall);
        
        printf("\n");
        Image newweak(weak);
        FCAM_IMAGE_DEBUG(weak);
        FCAM_IMAGE_DEBUG(newweak);
        printf("Destroying copies...\n");
    }
    FCAM_IMAGE_DEBUG(small);
    FCAM_IMAGE_DEBUG(weak);

    {
        printf("\nTesting assignment operator\n");
        
        Image newsmall = small;
        FCAM_IMAGE_DEBUG(small);
        FCAM_IMAGE_DEBUG(newsmall);

        Image newweak = weak;
        FCAM_IMAGE_DEBUG(weak);
        FCAM_IMAGE_DEBUG(newweak);

        printf("Destroying assigned copies...\n");
    }
    FCAM_IMAGE_DEBUG(small);
    FCAM_IMAGE_DEBUG(weak);
    
    {
        printf("\nTesting locking a copy\n");
        Image newsmall(small);
        Image newweak(weak);

        newsmall.lock();
        if (small.lock(0)) {
            printf("ERROR: should not have been able to acquire lock on original when copy is locked\n");
            FCAM_IMAGE_DEBUG(small);
            FCAM_IMAGE_DEBUG(newsmall);
            return -1;
        }
        newsmall.unlock();

        if (!small.lock(0)) {
            printf("ERROR: should be able to acquire lock on original when copy is unlocked\n");
            FCAM_IMAGE_DEBUG(small);
            FCAM_IMAGE_DEBUG(newsmall);
            return -1;
        }

        if (newsmall.lock(0)) {
            printf("ERROR: should be not able to acquire lock on copy where original is locked\n");
            FCAM_IMAGE_DEBUG(small);
            FCAM_IMAGE_DEBUG(newsmall);
            return -1;
        }

        printf("\nTesting destroying an unlocked copy while original is locked\n");
        weak.lock();
    }

    weak.unlock();
    small.unlock();
    FCAM_IMAGE_DEBUG(small);
    FCAM_IMAGE_DEBUG(weak);

    {
        printf("Testing making a copy, then setting the copies to special values\n");
        
        Image newsmall(small);
        newsmall = Image(newsmall.size(), newsmall.type(), Image::AutoAllocate);
        FCAM_IMAGE_DEBUG(small);
        FCAM_IMAGE_DEBUG(newsmall);

        Image newweak(weak);
        newweak = Image(newweak.size(), newweak.type(), Image::Discard);
        FCAM_IMAGE_DEBUG(weak);
        FCAM_IMAGE_DEBUG(newweak);

        printf("\nTesting deleting images with special values\n");
    }
    FCAM_IMAGE_DEBUG(small);
    FCAM_IMAGE_DEBUG(weak);

    {
        printf("\nTesting assigning myself to a copy\n");
        Image foo(small);
        small = foo;
        FCAM_IMAGE_DEBUG(small);
    }
    FCAM_IMAGE_DEBUG(small);

    {
        printf("\nTesting decref of assigned copies\n");
        for (int i = 0; i < 10; i++) {
            Image newsmall(small);
            newsmall = weak;
        }
    }
    FCAM_IMAGE_DEBUG(small);
    FCAM_IMAGE_DEBUG(weak);


    {
        printf("\nTesting assigning myself to myself\n");
        small = small;
    }
    FCAM_IMAGE_DEBUG(small);

    {
        printf("\nTesting making a backup, nuking myself, and restoring from backup\n");
        Image newsmall(small);
        small = Image();
        small = newsmall;

        Image newweak(weak);
        weak = Image();
        weak = newweak;
    }
    FCAM_IMAGE_DEBUG(small);
    FCAM_IMAGE_DEBUG(weak);

    printf("\nTiming creation and deletion of lots of images\n");
    Time start = Time::now();
    for (int i = 0; i < 1000; i++) {
        Image *a = new Image(640, 480, UYVY);
        Image *b = new Image(640, 480, UYVY);
        Image *c = new Image(640, 480, UYVY);
        Image *d = new Image(640, 480, UYVY);
        Image *e = new Image(640, 480, UYVY);
        delete e;
        delete d;
        delete c;
        delete b;
        delete a;
    }
    Time end = Time::now();
    printf("Took %d us per allocation and deletion\n", (end-start)/5000);

    printf("\nTesting image locking\n");
    {
        Image newsmall(small);
        small.lock();
        printf("This should not deadlock...\n");
        start = Time::now();
        if (newsmall.lock(10000)) {
            printf("ERROR: two different references to an image both successfully locked it\n");
            return 1;
        }
        end = Time::now();
        printf("Timeout on locking an image took %d us when it should take %d us\n", end-start, 10000);
        printf("Unlocking image\n");
        small.unlock();
        printf("Locking different reference and letting it fall out of scope\n");
        newsmall.lock();
    }
    printf("Relocking image\n");
    small.lock();
    small.unlock();


    printf("\nTesting subimages\n");

    FCAM_IMAGE_DEBUG(big);
    printf("  subImage1 = big.subImage(100,100,Size(100,100));\n");
    Image subImage1 = big.subImage(100,100,Size(100,100));
    FCAM_IMAGE_DEBUG(subImage1);
    printf("  subImage2 = big.subImage(500,100,Size(big.size.width,100));\n");
    Image subImage2 = big.subImage(500,100,Size(big.width(),100));
    FCAM_IMAGE_DEBUG(subImage2);
    
    printf("\nTesting image copy\n");
    FCAM_IMAGE_DEBUG(small);
    printf("  small = weak.copy()\n");
    small = weak.copy();
    FCAM_IMAGE_DEBUG(small);
    
    printf("\nTesting image copyFrom\n");
    *small(0,0)=100;
    *weak(0,0)=150;
    FCAM_IMAGE_DEBUG(small);
    printf("  small(0,0)=%d, weak(0,0)=%d\n", *small(0,0), *weak(0,0));
    printf("  small.copyFrom(weak)\n");
    small.copyFrom(weak);
    FCAM_IMAGE_DEBUG(small);
    printf("  small(0,0)=%d, weak(0,0)=%d\n", *small(0,0), *weak(0,0));

    printf("\nTesting subImage assignment\n");
    FCAM_IMAGE_DEBUG(subImage1);
    FCAM_IMAGE_DEBUG(subImage2);
    printf("  subImage1 = subImage2\n");
    subImage1 = subImage2;
    FCAM_IMAGE_DEBUG(subImage1);
    
    printf("  subImage1 = small\n");
    subImage1 = small;
    FCAM_IMAGE_DEBUG(subImage1);
    FCAM_IMAGE_DEBUG(subImage2);
    FCAM_IMAGE_DEBUG(small);

    printf("Success!\n");
    return 0; 
}
