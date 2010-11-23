#include <FCam/Dummy.h>
#include <stdio.h>

int main(int argc, char **argv) {

    FCam::Dummy::Sensor sensor;

    FCam::Dummy::Shot shot;
    shot.testPattern = FCam::Dummy::CHECKERBOARD;
    shot.exposure = 5000;
    shot.gain = 1.0f;
    shot.image = FCam::Image(sensor.maxImageSize(), FCam::RAW);
    shot.histogram.enabled = true;
    shot.histogram.region = FCam::Rect(0,0,sensor.maxImageSize().width, sensor.maxImageSize().height);
    shot.sharpness.enabled = false;

    sensor.capture(shot);
    FCam::Dummy::Frame frame = sensor.getFrame();

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
            printf ("Error capturing a frame\n");
            return 1;
        }
    }

    frame["lens.aperture"] = 1.8f;
    frame["testInt"] = 1;
    frame["testFloat"] = 3.14159;
    frame["testTime"] = FCam::Time::now();
    std::vector<std::string> testStrings;
    testStrings.push_back("string1");
    testStrings.push_back("string2");
    testStrings.push_back("string3");
    testStrings.push_back("string4");
    frame["testStrings"] = testStrings;


    std::string test1DNGName("testDNG_1.dng");
    std::string test1DumpName("testDump_1.tmp");
    std::string test2DNGName("testDNG_2.dng");
    std::string test2DumpName("testDump_2.tmp");
    std::string test2DumpThumbName("testDumpThumb_2.tmp");

    FCAM_FRAME_DEBUG(frame);

    printf("Saving test frame dump as %s\n", test1DumpName.c_str());
    FCam::saveDump(frame, test1DumpName);

    printf("Saving test frame DNG as %s\n", test1DNGName.c_str());

    FCam::saveDNG(frame, test1DNGName);
    if (FCam::getNextEvent(&e, FCam::Event::Error)) {
        do {
            if (e.type == FCam::Event::Error) {
                errors = true;
                printf("** FCam error [%d] %d at %s: %s\n", e.type, e.data, e.time.toString().c_str(), e.description.c_str());
            }
        } while(FCam::getNextEvent(&e, FCam::Event::Error));
        if (errors) {
            printf("Error saving DNG, exiting.\n");
            return 1;
        }
    }

    printf("Loading back DNG\n");
    FCam::DNGFrame fLoaded = FCam::loadDNG(test1DNGName);

    FCAM_FRAME_DEBUG(fLoaded);

    if (!fLoaded.valid()) {
        printf("Error loading test DNG file. Listing all errors:\n");

        while (FCam::getNextEvent(&e, FCam::Event::Error)) {
            printf("** FCam error [%d] %d at %s: %s\n", e.type, e.data, e.time.toString().c_str(), e.description.c_str());
        }
        return 1;
    }

    printf("Saving loaded DNG as %s\n", test2DumpName.c_str());
    FCam::saveDump(fLoaded, test2DumpName);
    if (FCam::getNextEvent(&e, FCam::Event::Error)) {
        do {
            if (e.type == FCam::Event::Error) {
                errors = true;
                printf("** FCam error [%d] %d at %s: %s\n", e.type, e.data, e.time.toString().c_str(), e.description.c_str());
            }
        } while (FCam::getNextEvent(&e, FCam::Event::Error));
        if (errors) {
            printf ("Error saving JPEG.\n");
            return 1;
        }
    }

    printf("Saving loaded DNG thumbnail as %s\n", test2DumpThumbName.c_str());
    FCam::saveDump(fLoaded.thumbnail(), test2DumpThumbName);
    if (FCam::getNextEvent(&e, FCam::Event::Error)) {
        do {
            if (e.type == FCam::Event::Error) {
                errors = true;
                printf("** FCam error [%d] %d at %s: %s\n", e.type, e.data, e.time.toString().c_str(), e.description.c_str());
            }
        } while (FCam::getNextEvent(&e, FCam::Event::Error));
        if (errors) {
            printf ("Error saving JPEG.\n");
            return 1;
        }
    }

    printf("Saving loaded DNG as %s\n", test2DNGName.c_str());
    FCam::saveDNG(fLoaded, test2DNGName);
    if (FCam::getNextEvent(&e, FCam::Event::Error)) {

        do {
            if (e.type == FCam::Event::Error) {
                errors = true;
                printf("** FCam error [%d] %d at %s: %s\n", e.type, e.data, e.time.toString().c_str(), e.description.c_str());
            }
        } while (FCam::getNextEvent(&e, FCam::Event::Error));
        if (errors) {
            printf ("Error saving DNG again.\n");
            return 1;
        }
    }

    printf("Done with test. Compare two DNGs for equality.\n");

    return 0;
}
