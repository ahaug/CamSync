#ifndef EF232LENSDATABASE_H
#define EF232LENSDATABASE_H

#include <set>
#include <map>
#include <string>
#include <iostream>

#define DEFAULT_DATABASE_FILE "EF232LensDatabase.txt"

namespace FCam {

    // All lengths are in mm, apertures are in f-number*10
    struct EF232LensInfo {
        std::string name;
        unsigned int focalLengthMin;
        unsigned int focalLengthMax;

        unsigned int focusDistMin;
        unsigned int apertureMax;

        float focusSpeed;  // diopters/sec

        bool hasImageStabilization;
        bool hasFullTimeManual;

        typedef std::pair<unsigned int, unsigned int> apertureChange;
        typedef std::map<unsigned int, unsigned int>::iterator minApertureListIter;
        typedef std::map<unsigned int, unsigned int>::const_iterator minApertureListCIter;
        // Aperture minimum f-stop assumed to be a piecewise-constant function of
        // focal length
        std::map<unsigned int, unsigned int> minApertureList;

        unsigned int minApertureAt(unsigned int focusDistance) const;
        bool operator<(const EF232LensInfo &rhs) const;

        void print(std::ostream &out) const;

        EF232LensInfo();
    };

    class EF232LensDatabase {
    public:
        EF232LensDatabase(const std::string &srcFile=DEFAULT_DATABASE_FILE);

        const EF232LensInfo* find(unsigned int focalLengthMin, 
                                  unsigned int focalLengthMax);

        const EF232LensInfo* find(const EF232LensInfo &key);

        const EF232LensInfo* update(const EF232LensInfo &lensInfo);

        void save(const std::string &dstFile=DEFAULT_DATABASE_FILE) const;
    private:
    
        void load(const std::string &srcFile);
    
        static std::set<EF232LensInfo> *db;
    };



}

#endif
