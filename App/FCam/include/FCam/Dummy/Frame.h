#ifndef FCAM_DUMMY_FRAME_H
#define FCAM_DUMMY_FRAME_H

#include "../Frame.h"
#include "Shot.h"

namespace FCam { namespace Dummy {
    
    struct _Frame: public FCam::_Frame, public FCam::Platform {
        TestPattern testPattern;
        std::string srcFile;
        
        FCam::Dummy::Shot _shot;
        
        BayerPattern _bayerPattern;
        unsigned short _minRawValue;
        unsigned short _maxRawValue;
        std::string _manufacturer;
        std::string _model;
    
        float rawToRGB3200K[12];
        float rawToRGB7000K[12];

        // A dummy frame acts as its own platform, so you can vary
        // these things on a per-frame basis while testing
        const FCam::Platform &platform() const {return *this;}

        const FCam::Dummy::Shot &shot() const { return _shot; }
        const FCam::Shot &baseShot() const { return shot(); }

        // Derived frames should implement these to return static
        // platform data necessary for interpreting this frame.
        BayerPattern bayerPattern() const { return _bayerPattern; }
        unsigned short minRawValue() const { return _minRawValue; }
        unsigned short maxRawValue() const { return _maxRawValue; }
        void rawToRGBColorMatrix(int kelvin, float *matrix) const;

        const std::string &manufacturer() const { return _manufacturer; }
        const std::string &model() const { return _model; }

        _Frame();
    };

    class Frame: public FCam::Frame {
    protected:
        const _Frame *get() const { return static_cast<_Frame*>(ptr.get()); }

    public:

        Frame(_Frame *f=NULL): FCam::Frame(f) {}

        TestPattern testPattern() const { return get()->testPattern; }
        const std::string &srcFile() const { return get()->srcFile; }
    
        const FCam::Dummy::Shot &shot() const {
            return get()->shot();
        }
    };
}}

#endif
