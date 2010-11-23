#include <FCam/Dummy/Shot.h>

namespace FCam { namespace Dummy {
    Shot::Shot(): FCam::Shot(), testPattern(BARS), srcFile("") {}
    Shot::Shot(const FCam::Shot &shot): FCam::Shot(shot), testPattern(BARS), srcFile("") {}
    Shot::Shot(const Shot &shot): FCam::Shot(shot), testPattern(shot.testPattern), srcFile(shot.srcFile) {}

    const Shot &Shot::operator=(const FCam::Shot &shot) { 
        FCam::Shot::operator=(shot);
        testPattern = BARS;
        srcFile = "";
        return *this;
    }

    const Shot &Shot::operator=(const Shot &shot) { 
        FCam::Shot::operator=(shot);
        testPattern = shot.testPattern;
        srcFile = shot.srcFile;
        return *this;
    }
}}
