#include <fstream>
#include <iostream>
#include <sstream>

#include "FCam/F2/EF232LensDatabase.h"
#include "../Debug.h"

#define eprintf(...) \
  fprintf(stderr,"EF232[LensDatabase]: ERROR! ");        \
  fprintf(stderr, __VA_ARGS__);

namespace FCam {

  unsigned int EF232LensInfo::minApertureAt(unsigned int focusDistance) const {
    if (minApertureList.size() == 0) return 0;
    std::map<unsigned int, unsigned int>::const_iterator iter=minApertureList.upper_bound(focusDistance);
    if (iter != minApertureList.begin())
      iter--;
    return iter->second;
  }

  bool EF232LensInfo::operator<(const EF232LensInfo &rhs) const {
    if (focalLengthMin == rhs.focalLengthMin) {
      return focalLengthMax < rhs.focalLengthMax;
    } else {
      return focalLengthMin < rhs.focalLengthMin;
    }
  }

  void EF232LensInfo::print(std::ostream &out) const {
    out << "{" << name << "}\n";
    out << "focalLengthMin=" << focalLengthMin << std::endl;
    out << "focalLengthMax=" << focalLengthMax << std::endl;
    out << "focusDistMin=" << focusDistMin << std::endl;
    out << "apertureMax=" << apertureMax << std::endl;
    out << "focusSpeed=" << focusSpeed << std::endl;
    out << "hasImageStabilization=" << hasImageStabilization << std::endl;
    out << "hasFullTimeManual=" << hasFullTimeManual << std::endl;
    for (minApertureListCIter iter = minApertureList.begin(); 
         iter != minApertureList.end(); iter++) {
      out << "aperture["<<iter->first<<"]="<<iter->second<<std::endl;
    }
    out << std::endl;
  }

  EF232LensInfo::EF232LensInfo(): 
    name("Unknown"), focalLengthMin(0), focalLengthMax(0),
    focusDistMin(0), apertureMax(0), focusSpeed(0),
    hasImageStabilization(false),
    hasFullTimeManual(false)
  { 
  }

  EF232LensDatabase::EF232LensDatabase(const std::string &srcFile) {
    if (!db) {
      db = new std::set<EF232LensInfo>;
      load(srcFile);
    } 
  }

  const EF232LensInfo* EF232LensDatabase::find(unsigned int focalLengthMin,
                                         unsigned int focalLengthMax)  {
    EF232LensInfo key;
    key.focalLengthMin = focalLengthMin;
    key.focalLengthMax = focalLengthMax;
    return find(key);
  }

  const EF232LensInfo* EF232LensDatabase::find(const EF232LensInfo &key) {
    std::set<EF232LensInfo>::iterator iter = db->find(key);
    if (iter == db->end()) iter = db->insert(db->begin(), key);
    return &*iter;
  }

  const EF232LensInfo* EF232LensDatabase::update(const EF232LensInfo &lensInfo) {
    std::set<EF232LensInfo>::iterator iter = db->find(lensInfo);
    if (iter != db->end()) db->erase(iter++);    
    return &*(db->insert(iter, lensInfo));
  }

  void EF232LensDatabase::load(const std::string &srcFile) {
    std::ifstream dbFile(srcFile.c_str());

    if (!dbFile.is_open()) {
      dprintf(1, "Unable to open database file %s\n", srcFile.c_str());    
      return;
    }

    // Very simple format for db file, very stupid parser for it!
    // # comment line
    // {lens name string}
    // <attribute> = <value>
    // ...
    // aperture[<focal_length>] = <aperture>
    // ...
    // [lens name string #2]

    std::string line;    
    std::size_t pos1,pos2;
    int lineNum = 0;
    EF232LensInfo *newLens = NULL;
    while(!dbFile.eof()) {
      std::getline(dbFile, line);
      lineNum++;
      line = line.substr(0, line.find_first_of("#")); // Remove comments
      
      if ( (pos1 = line.find_first_of('{') != line.npos) ) {        
        if (newLens != NULL) {
          db->insert(*newLens);
          delete newLens;
        }

        pos2=line.find_first_of('}');
        if (pos2 == line.npos) {
          eprintf("Malformed database entry on line %d: %s\n", lineNum, line.c_str());
          return;
        }
        newLens = new EF232LensInfo;
        newLens->name = line.substr(pos1, pos2-pos1);
        dprintf(1, "Reading in lens information for lens '%s'\n", newLens->name.c_str());
      }

      if ( (pos1 = line.find_first_of('=')) != line.npos ) {
        std::string attribute = line.substr(0, pos1);
        std::stringstream value(line.substr(pos1+1));
        dprintf(1, "  Attribute: %s, value: %s\n", attribute.c_str(), value.str().c_str());
        value >> std::ws;
        if (attribute.find("focalLengthMin") != attribute.npos) {
          value >> newLens->focalLengthMin;
        } else if (attribute.find("focalLengthMax") != attribute.npos) {
          value >> newLens->focalLengthMax;
        } else if (attribute.find("focusDistMin") != attribute.npos)  {
          value >> newLens->focusDistMin;
        } else if (attribute.find("apertureMax") != attribute.npos) {
          value >> newLens->apertureMax;
        } else if (attribute.find("focusSpeed") != attribute.npos) {
          value >> newLens->focusSpeed;
        } else if (attribute.find("hasImageStabilization") != attribute.npos) {
          value >> newLens->hasImageStabilization;
        } else if (attribute.find("hasFullTimeManual") != attribute.npos ){
          value >> newLens->hasFullTimeManual;
        } else if (attribute.find("aperture") != attribute.npos ){
          pos1 = attribute.find_first_of('[');
          pos2 = attribute.find_first_of(']',pos1);
          if (pos1 == attribute.npos || pos2 == attribute.npos) {
            eprintf("Malformed database entry on line %d: %s\n", lineNum, line.c_str());
            return;
          }
          std::stringstream index(attribute.substr(pos1+1, pos2-pos1-1));
          index >> std::ws;
          EF232LensInfo::apertureChange newApertureChange;
          index >> newApertureChange.first;
          value >> newApertureChange.second;
          newLens->minApertureList.insert(newApertureChange);
        } else {
          eprintf("Ignoring unknown database field %s on line %d: %s\n", attribute.c_str(), lineNum, line.c_str());
        }
      }
    }
    if (newLens != NULL) {
      db->insert(*newLens);
      delete newLens;
    }
  }

  void EF232LensDatabase::save(const std::string &dstFile) const {
    std::ofstream dbFile(dstFile.c_str());
    if (!dbFile.is_open()) {
      eprintf("Unable to open database file %s for writing!\n", dstFile.c_str());
      return;
    }

    dbFile << "########################\n";
    dbFile << "# EF-232 Lens Database file for Canon EOS lens parameters\n";
    dbFile << "# Auto-generated by EF232LensDatabase.cpp\n";
    dbFile << "# Units in mm, f/stops, or 0=false, 1=true\n";
    dbFile << "\n\n";

    for (std::set<EF232LensInfo>::iterator dbIter = db->begin();
         dbIter != db->end();
         dbIter++) {
      dbIter->print(dbFile);
    }

    dbFile <<"\n\n";
    dbFile <<"# End autogenerated lens database file\n";
  }

  std::set<EF232LensInfo> *EF232LensDatabase::db = NULL;

}
