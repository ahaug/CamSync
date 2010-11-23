#include <algorithm>

#include "FCam/F2/Shot.h"
#include "FCam/F2/Sensor.h"

#include "../Debug.h"
#include "linux/mt9p031.h"


namespace FCam{ namespace F2{

    Shot::Shot(): FCam::Shot(), 
                  rowSkip(RowSkip::none), colSkip(ColSkip::none),
                  rowBin(RowBin::none), colBin(ColBin::none),
                  roiCentered(false), roiStartX(0), roiStartY(0)
    {
    }

    Shot::Shot(const FCam::Shot &shot): FCam::Shot(shot) {
        // Need to create a F2::Shot from a base FCam::Shot.
        // Assume the entire image frame is desired.
        roiRegionSmaller(Sensor::activeArrayRect());
    }

    Shot::Shot(const Shot &other): FCam::Shot(static_cast<const FCam::Shot&>(other)),
                                   rowSkip(other.rowSkip),
                                   colSkip(other.colSkip),
                                   rowBin(other.rowBin),
                                   colBin(other.colBin),
                                   roiCentered(other.roiCentered),
                                   roiStartX(other.roiStartX),
                                   roiStartY(other.roiStartY)
    {
        // The FCam::Shot constructor should have taken care of the id increment,
        // all other fields are considered valid
    }

    const Shot &Shot::operator=(const FCam::Shot &other) {        
        FCam::Shot::operator=(other);
        // Assume the entire image frame is desired.
        roiRegionSmaller(Sensor::activeArrayRect());

        return *this;
    }

    const Shot &Shot::operator=(const Shot &other) {
        FCam::Shot::operator=(static_cast<const FCam::Shot&>(other));
        
        rowSkip = other.rowSkip;
        colSkip = other.colSkip;
        rowBin = other.rowBin;
        colBin = other.colBin;

        roiCentered = other.roiCentered;
        roiStartX = other.roiStartX;
        roiStartY = other.roiStartY;

        return *this;
    }

    void Shot::roiRegionSmaller(const Rect &maxRegion, bool useBinning) {
        // Won't take real pixel array limits into account here, just the current requested size

        int scaleX = maxRegion.width / image.width();  // Truncate down to be smaller than requested
        int scaleY = maxRegion.height / image.height();

        scaleX = std::max(1, std::min(scaleX, 7));
        scaleY = std::max(1, std::min(scaleY, 8));

        colSkip = static_cast<ColSkip::e>(scaleX);
        rowSkip = static_cast<RowSkip::e>(scaleY);
        
        if (useBinning) {
            int binX = std::max(1, std::min(scaleX, 4));
            int binY = std::max(1, std::min(scaleY, 4));

            if (binX == 3) binX = 2;

            colBin = static_cast<ColBin::e>(binX);
            rowBin = static_cast<RowBin::e>(binY);
        } else {
            colBin = ColBin::none;
            rowBin = RowBin::none;
        }

        roiCentered = false;
        roiStartX = maxRegion.x;
        roiStartY = maxRegion.y;               
    }

    void Shot::roiRegionSmaller(const Size &maxSize, bool useBinning) {
        FCam::Rect region(0,0, maxSize.width, maxSize.height);
        roiRegionSmaller(region, useBinning);
    }

    void Shot::roiRegionLarger(const Rect &minRegion, bool useBinning) {
        // Won't take real pixel array limits into account here, just the current requested size

        int scaleX = (minRegion.width / image.width()) + 1;  // Round up to be larger than requested
        int scaleY = (minRegion.height / image.height()) + 1;

        scaleX = std::max(1, std::min(scaleX, 7));
        scaleY = std::max(1, std::min(scaleY, 8));

        colSkip = static_cast<ColSkip::e>(scaleX);
        rowSkip = static_cast<RowSkip::e>(scaleY);

        if (useBinning) {
            int binX = std::max(1, std::min(scaleX, 4));
            int binY = std::max(1, std::min(scaleY, 4));

            if (binX == 3) {
                binX = 4;  // Round up to ensure region is larger than requested
                colSkip = ColSkip::x4; // Skip must at least equal bin
            }

            colBin = static_cast<ColBin::e>(binX);
            rowBin = static_cast<RowBin::e>(binY);
        } else {
            colBin = ColBin::none;
            rowBin = RowBin::none;
        }

        roiCentered = false;
        roiStartX = minRegion.x;
        roiStartY = minRegion.y;
    }

    void Shot::roiRegionLarger(const Size &minSize, bool useBinning) {
        FCam::Rect region(0,0, minSize.width, minSize.height);
        roiRegionLarger(region, useBinning);
    }

}}
