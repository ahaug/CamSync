//!\file
// Simple utility to manipulate FCam-produced DNG files
// 

#if !defined(FCAM_VERSION)
#define FCAM_VERSION "x.x.x"
#endif

#include <FCam/FCam.h>
#include <stdio.h>
#include <unistd.h>

void usage() {
    printf("fcamDngUtil\n"
           "  (FCam version " FCAM_VERSION ")\n"
           "  A simply utility to manipulate DNGs saved by programs using\n"
           "  the FCam API.\n\n"

           "Usage: fcamDngUtil [FLAGS] INPUT [OUTPUT].\n"
           "  With no output name given, adds .up. before the .dng extension to\n"
           "  the input to create the output name for DNG output, and uses the\n"
           "  input file name with the .jpg extension for JPEG.\n"
           "Flags:\n"
           "  -i Dump out information about the DNG contents\n"
           "  -u Write a new DNG with metadata updated to the latest version.\n"
           "  -j Write a JPEG file created from the input DNG.\n" 
           "     If -u is also specified, only -j takes effect.\n"
           "  -c Specify a different contrast amount for JPEG conversion\n"
           "     (default = 50, range 0-100, larger numbers increase contrast.)\n"
           "  -b Specify a different black level for JPEG conversion (default = 25).\n"
           "  -g Specify a different gamma value for JPEG conversion (default = 2.2).\n"
           "  -q Specify JPEG quality (default = 80, range 0-100).\n"
           "  -n Turn off hot pixel suppression.\n"
           "\n");
    exit(1);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
    }

    bool dumpFrame = false;
    bool writeDNG = false;
    bool writeJPEG = false;

    bool denoise = true;
    int contrast = 50;
    int blackLevel = 25;
    float gamma = 2.2f;
    int quality = 80;

    const char *options="iujc:b:g:nq:";
    
    int argChar;
    while (  (argChar = getopt(argc,argv,options)) != -1) {
        switch (argChar) {
        case 'i':
            dumpFrame = true;
            break;
        case 'u':
            writeDNG = true;
            break;
        case 'j':
            writeJPEG = true;
            break;
        case 'c':
            sscanf(optarg,"%d", &contrast);
            break;
        case 'b':
            sscanf(optarg,"%d", &blackLevel);
            break;
        case 'g':
            sscanf(optarg,"%f", &gamma);
            break;
        case 'n':
            denoise = false;
            break;
        case 'q':
            sscanf(optarg,"%d", &quality);
            break;
        case '?':
            usage();
            break;
        }
    }

    if (optind == argc) {
        printf("!! Error: No input file given!\n\n");
        usage();
    }

    std::string inputFile(argv[optind]);

    std::string outputFile;
    if (optind < argc -1 ) {
        outputFile = std::string(argv[optind+1]);
    } else {
        if (writeJPEG) {
            outputFile = inputFile.substr(0,inputFile.rfind(".dng"));
            outputFile.append(".jpg");
        } else {
            outputFile = inputFile.substr(0,inputFile.rfind(".dng"));
            outputFile.append(".up.dng");
        }
    }

    if (!dumpFrame && !writeJPEG && !writeDNG) {
        printf("!! Error: Nothing to do\n\n");
        usage();
    }

    FCam::Event e;

    printf("  Reading input DNG %s\n", inputFile.c_str());

    FCam::DNGFrame f = FCam::loadDNG(inputFile);

    if (FCam::_eventQueue.size() > 0) {
        printf("  Events reported while loading DNG:\n");
        while (getNextEvent(&e)) {
            switch (e.type) {
            case FCam::Event::Error:
                printf("    Error code: %d. Description: %s\n", e.data, e.description.c_str());
                break;
            case FCam::Event::Warning:
                printf("    Warning code: %d. Description: %s\n", e.data, e.description.c_str());
                break;
            default:
                printf("    Event type %d, code: %d. Description: %s\n", e.type, e.data, e.description.c_str());
                break;
            }
        }
    }

    if (!f.valid()) {
        printf("!! Error: Unable to read input file %s\n", inputFile.c_str());
        exit(1);
    }

    if (dumpFrame) {
        f.debug(inputFile.c_str());
    }

    if (writeJPEG) {
        printf("  Demosaicing with contrast %d, black level %d, gamma %f %s\n", contrast, blackLevel, gamma, denoise ? "":"(Denoising off)");
        FCam::Image jpegImg = FCam::demosaic(f, contrast, denoise, blackLevel, gamma);        
        if (FCam::_eventQueue.size() > 0) {
            printf("Events reported while demosaicing DNG:\n");
            while (getNextEvent(&e)) {
                switch (e.type) {
                case FCam::Event::Error:
                    printf("  Error code: %d. Description: %s\n", e.data, e.description.c_str());
                    break;
                case FCam::Event::Warning:
                    printf("  Warning code: %d. Description: %s\n", e.data, e.description.c_str());
                    break;
                default:
                    printf("  Event type %d, code: %d. Description: %s\n", e.type, e.data, e.description.c_str());
                    break;
                }
            }
        }        
        if (!jpegImg.valid()) {
            printf("!! Error: Unable to demosaic input file %s\n", inputFile.c_str());
            exit(1);
        }
        printf("  Writing to %s, quality %d\n", outputFile.c_str(), quality);
        saveJPEG(jpegImg, outputFile, quality);

        if (FCam::_eventQueue.size() > 0) {
            printf("Events reported while saving JPEG:\n");
            while (getNextEvent(&e)) {
                switch (e.type) {
                case FCam::Event::Error:
                    printf("  Error code: %d. Description: %s\n", e.data, e.description.c_str());
                    break;
                case FCam::Event::Warning:
                    printf("  Warning code: %d. Description: %s\n", e.data, e.description.c_str());
                    break;
                default:
                    printf("  Event type %d, code: %d. Description: %s\n", e.type, e.data, e.description.c_str());
                    break;
                }
            }
        }
              
    } else if (writeDNG) {
        printf("  Writing updated DNG to %s\n", outputFile.c_str());
        FCam::saveDNG(f, outputFile);
        
        if (FCam::_eventQueue.size() > 0) {
            printf("  Events reported while saving to %s:\n", outputFile.c_str());
            while (getNextEvent(&e)) {
                switch (e.type) {
                case FCam::Event::Error:
                    printf("    Error code: %d. Description: %s\n", e.data, e.description.c_str());
                    break;
                case FCam::Event::Warning:
                    printf("    Warning code: %d. Description: %s\n", e.data, e.description.c_str());
                    break;
                default:
                    printf("    Event type %d, code: %d. Description: %s\n", e.type, e.data, e.description.c_str());
                    break;
                }
            }
        }
    }
    printf("  Done!\n");
    return 0;
}
