// A simple test program to verify timing and functionality of the F2 under FCAM

#include "FCam/F2.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <stdio.h>

typedef std::vector<FCam::Shot> Burst;

void roiTimingTest() {
    using namespace FCam::F2;
    using namespace std;
    char buf[256];
    FILE *fp;

    printf("roiTimingTest: Tabulating frame time changes with ROI\n"
           "-------------------------------------------\n");

    fp = fopen("roiTiming.csv", "w");
    if (fp == NULL) {
        printf("Unable to open stats file roiTiming.csv, exiting\n");
        return;
    }

    int e[] = {1000,5000,10000,20000,40000,80000};
    vector<int> exposures(e, e + sizeof(e)/sizeof(int));

    int ft[] = {15000,20000,40000,80000};
    vector<int> frameTimes(ft, ft + sizeof(ft)/sizeof(int));

//     RowSkip::e rs[] = { RowSkip::none, RowSkip::x2, RowSkip::x3, RowSkip::x4,
//                         RowSkip::x5, RowSkip::x6, RowSkip::x7, RowSkip::x8 };
    RowSkip::e rs[] = { RowSkip::none, RowSkip::x2, RowSkip::x4};

    vector<RowSkip::e> rowSkips(rs, rs + 3);

//     ColSkip::e cs[] = { ColSkip::none, ColSkip::x2, ColSkip::x3, ColSkip::x4,
//                               ColSkip::x5, ColSkip::x6, ColSkip::x7 };
    ColSkip::e cs[] = { ColSkip::none, ColSkip::x2, ColSkip::x4};

    vector<ColSkip::e> colSkips(cs, cs + 3);

//    RowBin::e rb[] = { RowBin::none, RowBin::x2, RowBin::x3, RowBin::x4 };
    RowBin::e rb[] = { RowBin::none, RowBin::x2, RowBin::x4 };
    vector<RowBin::e> rowBins(rb, rb + 3);

    ColBin::e cb[] = { ColBin::none, ColBin::x2, ColBin::x4 };
    vector<ColBin::e> colBins(cb, cb + 3);    

    vector<bool> changeRoiXY(2);
    changeRoiXY[0] = false;
    changeRoiXY[1] = true;

    int roiXstd = 0;
    int roiYstd = 0;
    int roiXchg = 500;
    int roiYchg = 500;

    int dstRealFT, srcRealFT;
    int dstRealExp, srcRealExp;

    Shot srcShot, dstShot;
    srcShot.gain = 8;
    srcShot.image = FCam::Image(640,480,FCam::UYVY, FCam::Image::Discard);
    dstShot = srcShot; 

    Sensor sensor;

    unsigned int burstCount = 10;            
    unsigned int n = 6;

    snprintf(buf, 256, "sExp(ms), sFT(ms), sRS, sCS, sRB, sCB, sSX, sSY,   dExp(ms), dFT(ms), dRS, dCS, dRB, dCB, dSX, dSY");
    printf(buf); fprintf(fp, buf);
    for (unsigned int k=0; k <n ; k++) {
        snprintf(buf, 256, ",   exp_ms[%d], ft_ms[%d], avg_dT_ms[%d], std_dT_us[%d], cnt[%d]", k,k,k,k,k);
        printf(buf); fprintf(fp, buf);
    }
    snprintf(buf, 256, "\n");
    printf(buf); fprintf(fp,buf);

    for (vector<int>::iterator fTime = frameTimes.begin(); fTime != frameTimes.end(); fTime++) {
    for (vector<int>::iterator exp = exposures.begin(); exp != exposures.end(); exp++) {
        //    for (vector<bool>::iterator rXY = changeRoiXY.begin(); rXY != changeRoiXY.end(); rXY++) {
    for (int sb=0; sb < 3; sb++) {
        srcShot.roiStartX = roiXstd;
        srcShot.roiStartY = roiYstd;
        srcShot.rowSkip = rowSkips[sb]; srcShot.colSkip = colSkips[sb];
        srcShot.rowBin = rowBins[sb]; srcShot.colBin = colBins[sb];
        srcShot.frameTime = *fTime;
        srcShot.exposure = *exp;
        for (vector<int>::iterator fTime2 = frameTimes.begin(); fTime2 != frameTimes.end(); fTime2++) {
        for (vector<int>::iterator exp2 = exposures.begin(); exp2 != exposures.end(); exp2++) {
        for (int sb2 =0; sb2< 3; sb2++) {
            dstShot.roiStartX = roiXstd; //*rXY ? roiXchg : roiXstd;
            dstShot.roiStartY = roiYstd; //*rXY ? roiYchg : roiYstd;
            dstShot.rowSkip = rowSkips[sb2]; dstShot.colSkip = colSkips[sb2];
            dstShot.rowBin = rowBins[sb2]; dstShot.colBin = colBins[sb2];
            dstShot.frameTime = *fTime2;
            dstShot.exposure = *exp2;

            std::vector<Shot> testBurst(n);
            
            unsigned int i=0;
            for (; i < n/2; i++) {
                testBurst[i] = srcShot;
            }
            for (; i < n; i++) {
                testBurst[i] = dstShot;
            }
            int chgIndex = n/2;

            sensor.debugTiming(true);

            sensor.capture(dstShot); // Extra frame to allow for nice deltaTs
            for (unsigned int i=0; i< burstCount; i++) sensor.capture(testBurst);
            
            FCam::Time prevT;
            {
                Frame::Ptr f = sensor.getF2Frame();
                prevT = f->processingDoneTime;
            }
            vector<float> dT[n];
            float driverExp[n], driverFT[n];
            for (unsigned int i=0;i<n;i++) driverExp[i] = driverFT[i] = 0; 

            int testFrames = burstCount * n;
            int index = 0;
            while (testFrames-- > 0) {
                Frame::Ptr f = sensor.getF2Frame();
                
                float deltaT = (f->processingDoneTime - prevT) / 1000.0;
                prevT = f->processingDoneTime;
                
                dT[index].push_back(deltaT);
                driverExp[index] += f->exposure;
                driverFT[index] += f->frameTime;
                index = (index + 1) % n;
            }

            float avg[n];
            float std[n];
            for (unsigned int k=0;k<n;k++) {
                avg[k] = 0;
                for (unsigned int i=0; i < dT[k].size(); i++) 
                    avg[k] += dT[k][i];
                avg[k] /= dT[k].size();

                std[k] = 0;
                for (unsigned int i=0; i < dT[k].size(); i++) 
                    std[k] += (dT[k][i] - avg[k])*(dT[k][i] - avg[k]);
                std[k] = sqrt( std[k] / dT[k].size());
            }

            snprintf(buf,256, "%.2f, %.2f, %d, %d, %d, %d, %d, %d,    ",
                   srcShot.exposure/1000.f, srcShot.frameTime/1000.f, 
                   srcShot.rowSkip, srcShot.colSkip, srcShot.rowBin, srcShot.colBin, 
                   srcShot.roiStartX, srcShot.roiStartY);
            printf(buf); fprintf(fp, buf);

            snprintf(buf,256, "%.2f, %.2f, %d, %d, %d, %d, %d, %d",
                   dstShot.exposure/1000.f, dstShot.frameTime/1000.f, 
                   dstShot.rowSkip, dstShot.colSkip, dstShot.rowBin, dstShot.colBin, 
                   dstShot.roiStartX, dstShot.roiStartY);
            printf(buf); fprintf(fp, buf);
            for (unsigned int k=0; k < n; k++) {
                if ( k % 3 == 0) printf("\n\t");
                snprintf(buf,256, ",    %.1f, %.1f, %.2f, %.1f, %d", 
                         driverExp[k]/dT[k].size()/1000, driverFT[k]/dT[k].size()/1000,
                         avg[k], std[k]*1000, dT[k].size());
                printf(buf); fprintf(fp, buf);
            }
            snprintf(buf,256,"\n");
            printf(buf); fprintf(fp,buf);

            fflush(fp);
            if (sensor.framesPending()) {
                printf("!! Still got frames, that's not good\n");
            }

        }
        }
        }
    }
    }
    //    }
    }
}

void syncTest() {
    using namespace FCam::F2;
    using namespace std;

    printf("syncTest: Testing basic frame-level control\n"
           "-------------------------------------------\n");

    // Initialize a sensor
    Sensor sensor;
    unsigned int n = 10;

    // Create a n-image burst with one image with different parameters

    Burst testShots(1);

    testShots[0].exposure = 1000;
    testShots[0].gain = 10;
    testShots[0].frameTime = 40000;
    testShots[0].image = FCam::Image(640, 480, FCam::UYVY);

    for (unsigned int i=1; i < n;i++) {
        testShots.push_back(testShots[0]);
        testShots[i].image = FCam::Image(640,480, FCam::UYVY);
        if (i >= n/2) testShots[i].exposure = 20000; //25000;
    }
    testShots[n-1].exposure = 40000;

    sensor.stream(testShots);

    vector<vector<float> > lums(n);
    vector<vector<float> > deltaT(n);

    int testFrames = n*10;
    FCam::Time prevTime = FCam::Time::now();
    bool startup = true;
    int startupIgnoreCount = 0; //n-1;

    printf("* Capturing %d frames of a %d-shot burst\n", testFrames, n);
    while (testFrames-- > 0) {
        unsigned int index;
        FCam::Frame::Ptr f = sensor.getFrame();
        
        for (index=0; index<n ;index++ ) {
            if (testShots[index].id == f->shot().id) break;
        }
        if (index == n) {
            printf("Unknown frame returned! Something wrong in the shot cloning, perhaps?\n");
            exit(0);
        }

        if (startupIgnoreCount-- == 0)
            startup=false;

        if (!startup) {
            float dt = (f->processingDoneTime-prevTime) / 1000.;
            printf("Frame %d: Time from previous frame: %.2f ms, supposed to be %.2f\n", index, dt,
                   f->frameTime/1000.);
            deltaT[index].push_back(dt);
        } 

        prevTime = f->processingDoneTime;
            
        if (!f->image.valid()) {
            printf(" Frame %d Came back with no image data!\n", index);
            continue;
        } 

        // Calculate some statistics 
        unsigned int totalY=0;
        unsigned char *yPtr = f->image.data+1; // Offset to get to a Y
        unsigned int count=0;
        while (yPtr < f->image.data + 2*f->image.size.width*f->image.size.height) {
            totalY+= *yPtr;
            yPtr += 100;
            count++;
        }        
        lums[index].push_back( ((float)totalY)/count);
    }
    sensor.stopStreaming();

    printf("Writing stats to syncTest.csv\n");
    ofstream stats("syncTest.csv");   
    bool done = false;
    unsigned int row = 0;
    while (!done) {        
        int haveData=0;
        for (unsigned int i=0;i < n; i++) {
            if (row < lums[i].size()) {
                stats << lums[i][row] << ", ";
                haveData++;
            } else {
                stats << "-1 ,";
            }
            if (row < deltaT[i].size()) {
                stats << deltaT[i][row];
                haveData++;
            } else {
                stats << "-1";
            }
            if (i < n-1)
                stats << " ,";
        }
        stats << endl;
        if (haveData == 0) done = true;
        row++;
    }
    stats.close();

    printf("\n\n** Results (Y=sampled luminance per pixel, T=inter-frame time)\n\n");
    // Calculate averages, stddevs
    vector<float> avgsL(n), stddevsL(n), lowboundL(n), highboundL(n);
    vector<float> avgsT(n), stddevsT(n), lowboundT(n), highboundT(n);
    for (unsigned int i=0;i<n;i++) {
        avgsL[i] = 0;
        stddevsL[i] = 0;
        for (unsigned int j=0;j < lums[i].size(); j++) {
            avgsL[i] += lums[i][j];
        }
        avgsL[i] /= lums[i].size();
        for (unsigned int j=0;j < lums[i].size(); j++) {
            stddevsL[i] += (lums[i][j] - avgsL[i])*(lums[i][j] - avgsL[i]);
        }
        stddevsL[i] /= lums[i].size();
        stddevsL[i] = sqrt(stddevsL[i]);        
        sort(lums[i].begin(), lums[i].end());
        if (lums[i].size()>10) {
            lowboundL[i] = lums[i][lums[i].size()/10];
            highboundL[i] = lums[i][lums[i].size()*9/10];
        } else {
            lowboundL[i]=-1;
            highboundL[i]=-1;
        }
        printf("Shot %d cnt %d, Lum: Avg: %.1f, Std: %.1f, 10%%: %f, 90%%: %f\n", i, lums[i].size(), avgsL[i], stddevsL[i], lowboundL[i], highboundL[i]);
    }    
    printf("\n");
    for (unsigned int i=0;i<n;i++) {
        avgsT[i] = 0;
        stddevsT[i] = 0;
        for (unsigned int j=0;j < deltaT[i].size(); j++) {
            avgsT[i] += deltaT[i][j];
        }
        avgsT[i] /= deltaT[i].size();
        for (unsigned int j=0;j < deltaT[i].size(); j++) {
            stddevsT[i] += (deltaT[i][j] - avgsT[i])*(deltaT[i][j] - avgsT[i]);
        }
        stddevsT[i] /= deltaT[i].size();
        stddevsT[i] = sqrt(stddevsT[i]);
        sort(deltaT[i].begin(), deltaT[i].end());
        if (deltaT[i].size()>10){
            lowboundT[i] = deltaT[i][deltaT[i].size()/10];
            highboundT[i] = deltaT[i][deltaT[i].size()*9/10];
        } else {
            lowboundT[i] = -1;
            highboundT[i] = -1;
        }
        printf("Shot %d, Interframe delay: Avg: %.3f ms, Std: %.2f us, 10%%: %.2f, 90%%: %.2f Exp: %.1f ms\n",
               i, avgsT[i], stddevsT[i]*1000, lowboundT[i], highboundT[i], testShots[i].exposure/1000. );
    }

    printf("syncTest: Done\n"
           "-------------------------------------------\n");

}

void basicTest() {
    using namespace FCam;
    using namespace std;

    printf("basicTest: Testing basic capture in all formats\n"
           "-------------------------------------------\n");

    unsigned int n = 15;
    F2::Sensor sensor;
    F2::Shot testShot;
    AsyncFileWriter writer;

    testShot.exposure = 40000;
    testShot.frameTime = 0;
    testShot.gain = 8;
    
    // Get 640x480 UYVY images
    
    printf("=== 640x480 UYVY ===\n");
    testShot.image = Image(640,480,UYVY, Image::Discard);
    testShot.roiRegionSmaller(sensor.maxImageSize());
    for (int i=0;i < n;i++) sensor.capture(testShot);
    for (int i=0;i < n;i++) { sensor.getFrame(); printf("\tGot frame %d\n", i); }
    
    
    // Get 5 MP UYVY images
    printf("=== 5MP UYVY ===\n");
    testShot.image = Image(sensor.maxImageSize(), UYVY, Image::Discard);
    testShot.roiRegionSmaller(sensor.maxImageSize());
    {
        //Frame::Ptr f[n];
        for (int i=0;i < n;i++) sensor.capture(testShot);
        for (int i=0;i < n;i++) { sensor.getFrame(); printf("\tGot frame %d\n", i); }        
        //for (int i=0;i < n;i++) { f[i] = sensor.getFrame(); printf("\tGot frame %d\n", i); }
        //for (int i=0;i< n; i++) { char name[256]; snprintf(name,256, "basic_%02d.yuyv", i); saveUYVY(f[i], name); }
    }
    
    
    // Get 5 MP RAW images
    printf("=== 5MP RAW ===\n");
    testShot.image = Image(sensor.maxImageSize(), RAW, Image::Discard);
    testShot.roiRegionSmaller(sensor.maxImageSize());
    {
        //Frame::Ptr f[n];
        for (int i=0;i < n;i++) sensor.capture(testShot);
        for (int i=0;i < n;i++) { sensor.getFrame(); printf("\tGot frame %d\n", i); }        
        //for (int i=0;i < n;i++) { f[i]=sensor.getFrame(); printf("\tGot frame %d\n", i); }        
        //for (int i=0;i< n; i++) { char name[256]; snprintf(name,256, "basic_%02d.dng", i); saveDNG(f[i], name); }
    }
   
    // Get 640x480 RAW images
    printf("=== 640x480 RAW ===\n");
    //sensor.debugTiming(true);
    testShot.image = Image(640,480,RAW, Image::AutoAllocate);
    testShot.roiRegionSmaller(sensor.maxImageSize());
    for (int i=0;i < n;i++) sensor.capture(testShot);
    for (int i=0;i < n;i++) { sensor.getFrame(); printf("\tGot frame %d\n", i); }        

    // Get 640x480 UYVY images again
    
    printf("=== 640x480 UYVY again ===\n");
    testShot.image = Image(640,480,UYVY, Image::Discard);
    testShot.roiRegionSmaller(sensor.maxImageSize());
    for (int i=0;i < n;i++) sensor.capture(testShot);
    for (int i=0;i < n;i++) { sensor.getFrame(); printf("\tGot frame %d\n", i); }
    

    /*
    for (int i=0;i < n;i++) { 
        Frame::Ptr f=sensor.getFrame(); 
        
        printf("\tGot frame %d\n", i); 
        char name[256]; snprintf(name,256, "basic_%02d.dng", i); 
        writer.saveDNG(f, name);
    }
    */    
}

void usage() {
    printf("test_F2 Usage:\n\ntest_F2 <test1> <test2> ...\n");
    printf("Available tests:\n");
    printf("\tb\tBasic\tJust try capturing some frames in UYVY/RAW modes and 640x48/5 MP\n");
    printf("\ts\tSync\tTest timing of shot parameter changes\n");
    printf("\tr\tRoi Timing\tCollect a lot of statistics. Takes a while, writes roiTiming.csv as it goes\n");
}

int main(int argc, char **argv) {
    if (argc > 1) {
        printf("Starting F2 FCam API tests\n"
               "===============================\n");
        for (int i=1; i < argc; i++) {
            switch(argv[i][0]) {
            case 'b':
            case 'B':
                basicTest();
                break;
            case 's':
            case 'S':
                syncTest();
                break;
            case 'r':
            case 'R':
                roiTimingTest();
                break;
            default:
                printf("Unknown test %s\n", argv[0]);
                usage();
                break;
            };
        }
        printf("===============================\n"
               "Done with tests\n"
               );

    } else {
        usage();
    }
}
