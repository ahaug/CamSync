#include "../F2.h"

int main(int argc, char **argv) {
    FCam::F2::Lens lens;

    // Wait for lens to boot
    int counter = 0;
    while (lens.getState() != FCam::F2::Lens::Ready && counter < 20) {
        usleep(100000);
        counter++;
    }
    if (counter == 20) {
        if (lens.getState() == FCam::F2::Lens::NoLens) {
            std::cout << "No lens plugged in yet. How about you put one in and try again" << std::endl;
        } else {
            std::cout << "Lens isn't becoming ready. Probably a communication problem with the controller"<<std::endl;
        }
        exit(0);
    }

    lens.setFocus(lens.farFocus());
    std::cout << "Tried to focus at 0 diop, actual: "<<lens.getFocus() << std::endl;
    
    lens.setFocus(lens.nearFocus());
    std::cout << "Tried to focus at "<<lens.nearFocus()<<" diop, actual: "<<lens.getFocus() << std::endl;
    
    std::cout << "Lens focus speed range: " << lens.minFocusSpeed() << " to "<< lens.minFocusSpeed()<<" diopters/sec" << std::endl;
    
    std::cout << "Lens focal length: " << lens.getZoom() <<std::endl;
    
    lens.setAperture(lens.wideAperture());
    std::cout << "Tried to open aperture, actual: "<<lens.getAperture() << std::endl;
    
    lens.setAperture(lens.narrowAperture());
    std::cout << "Tried to close aperture, actual: "<<lens.getAperture() << std::endl;
    
    FCam::EF232LensDatabase db;
    
    const FCam::EF232LensInfo *currentLens = db.find(lens.minZoom(),
                                                     lens.maxZoom());
    
    std::cout << "Database for the current lens" <<std::endl;
    currentLens->print(std::cout);

    usleep(1000000);

    // Write out database if requested
    if (argc > 1) {
        std::string arg1(argv[1]);
        
        if (arg1 == "-f") {
            std::cout << "Testing focus movement speed"<<std::endl;
            float startDiop = lens.nearFocus();
            float endDiop = lens.farFocus();
            int steps = 10;
            for (float diop = startDiop; diop > endDiop; diop -= (startDiop-endDiop)/steps ) {
                lens.setFocus(startDiop);
                while (!lens.focusChanging()) usleep(1000);
                while (lens.focusChanging()) usleep(1000);
                FCam::Time startT = FCam::Time::now();                
                lens.setFocus(diop);
                while (!lens.focusChanging()) usleep(1000);
                while (lens.focusChanging()) usleep(1000);
                FCam::Time endT = FCam::Time::now();
                std::cout << "Moving from "<<startDiop<<" to "<<diop<<
                    " diopters took "<<(endT-startT)/1000.0<<" ms, which is "<<(startDiop-diop)/((endT-startT)/1000000.0)<<" diop/sec"<<std::endl;
            }
        } else if (arg1 == "-c") {
            std::cout << "Starting full lens calibration"<<std::endl;
            
            // Construct focal length->min aperture map
            std::cout << "Please move lens to the minimum focal length (largest field of view)" << std::endl;
            int timeout_count = 0;
            while (timeout_count < 50) {
                if (lens.getZoom() == lens.minZoom()) break;
                timeout_count++;
                usleep(100000);
            }
            if (timeout_count == 50) {
                std::cout << "Timeout on calibration, bye!\n";
                return 1;
            }
            
            std::cout << "Please move lens to the maximum focal length (smallest field of view)" << std::endl;
            timeout_count = 0;
            while (timeout_count < 500) {
                if (lens.getZoom() == lens.maxZoom()) break;
                timeout_count++;
                usleep(10000);
            }
            if (timeout_count == 50) {
                std::cout << "Timeout on calibration, bye!\n";
                return 1;
            }
            currentLens = db.find(lens.minZoom(),
                                  lens.maxZoom());
            
            
            if (currentLens->name == "Unknown") {        
                FCam::EF232LensInfo updatedLens = *currentLens;
                std::cout << "Please enter name for current lens: "<<std::endl;        
                std::getline(std::cin, updatedLens.name);
                currentLens = db.update(updatedLens);
            }
            
            std::cout << "Calibrated database for the current lens" <<std::endl;
            currentLens->print(std::cout);      
            
            std::cout << "Ok to save? (y/n)"<<std::endl;
            std::string tmp;
            std::cin>>std::ws;
            std::cin>>tmp;
            if (tmp == "y"){
                std::cout << "Saving database" << std::endl;
                db.save();
            }
        }
    }
}
