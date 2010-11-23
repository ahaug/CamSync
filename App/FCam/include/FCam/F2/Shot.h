#ifndef FCAM_F2_SHOT
#define FCAM_F2_SHOT
#include "../Shot.h"

//! \file
//! F2::Shot collects parameters for capturing an F2::Frame,
//! a superset of those of the base FCam::Shot.
//! These parameters include full region-of-interest control.

namespace FCam {
    namespace F2 {
        //! Number of rows to skip for each row read out.
        //! The row skip count must be at least as large as the bin count
        namespace RowSkip { 
            enum e {
                none = 1,
                x2   = 2,
                x3   = 3,
                x4   = 4,
                x5   = 5,
                x6   = 6,
                x7   = 7,
                x8   = 8
            };
        }
        //! Number of columns to skip for each column read out.
        //! The column skip count must be at least as large as the bin count
        namespace ColSkip {
            enum e {
                none = 1,
                x2   = 2,
                x3   = 3,
                x4   = 4,
                x5   = 5,
                x6   = 6,
                x7   = 7
            };
        }
        //! Number of rows to bin together for each row read out.
        //! The row skip count must be at least as large as the bin count
        namespace RowBin {
            enum e {
                none = 1,
                x2   = 2,
                x3   = 3,
                x4   = 4
            };
        }
        //! Number of columns to bin together for each column read out.
        //! The column skip count must be at least as large as the bin count
        namespace ColBin {
            enum e {
                none = 1,
                x2   = 2,
                x4   = 4
            };
        }
    
        /*! F2::Shot collects parameters for capturing a frame with
         * support for all the parameters of the F2 Frankencamera.
         * Most machinery inherited from FCam::Shot
         */
        class Shot:public FCam::Shot {
        public:
            //! Number of pixel rows skipped per row read out.
            //! If rowSkip is smaller than rowBin, it will increased to match
            RowSkip::e rowSkip;

            //! Number of pixel columns skipped per column read out.
            //! If colSkip is smaller than colBin, it will increased to match
            ColSkip::e colSkip;

            //! Number of pixel rows averaged together per row read out.
            //! If rowSkip is smaller than rowBin, it will increased to match
            RowBin::e rowBin;

            //! Number of pixel columns average together per column read out.
            //! If colSkip is smaller than colBin, it will increased to match
            ColBin::e colBin;

            /*! Flag to indicate that the region of interest should be
             * centered on the sensor's active pixel area
             */
            bool roiCentered;

            /*! If roiCentered is false, defines the top-left corner
             * of the region read out. 0 is the left edge of the active
             * area. Negative values result in readout of the black
             * pixels used for black level calibration.
             */
            int roiStartX;

            /*! If roiCentered is false, defines the top-left corner
             * of the region read out. 0 is the top edge of the active
             * area. Negative values result in readout of the black
             * pixels used for black level calibration.
             */
            int roiStartY;

            /*! Convenience function for setting skip/bin and
             *  roiStart.  Sets the region to be as close as possible
             *  to the requested region while being smaller than the
             *  requested region.  Defaults to turning on binning,
             *  which reduces frame rate but reduces image artifacts
             */
            void roiRegionSmaller(const Rect &maxRegion, bool useBinning = true);

            /*! Convenience function for setting skip/bin and
             *  roiStart.  Sets the region to be as close as possible
             *  to the requested size while being smaller than the
             *  requested size, with the top corner at (0,0) Defaults
             *  to turning on binning, which reduces frame rate but
             *  reduces image artifacts
             */
            void roiRegionSmaller(const Size &maxSize, bool useBinning = true);

            /*! Convenience function for setting skip/bin and
             *  roiStart.  Sets the region to be as close as possible
             *  to the requested region while being larger than the
             *  requested region.  Defaults to turning on binning,
             *  which reduces frame rate but reduces image artifacts
             */
            void roiRegionLarger(const Rect &minRegion, bool useBinning = true);

            /*! Convenience function for setting skip/bin and
             *  roiStart.  Sets the region to be as close as possible
             *  to the requested size while being larger than the
             *  requested size, with the top corner at (0,0) Defaults
             *  to turning on binning, which reduces frame rate but
             *  reduces image artifacts
             */
            void roiRegionLarger(const Size &minSize, bool useBinning = true);
        

            Shot();
            Shot(const FCam::Shot &shot);
            Shot(const Shot &shot);

            const Shot &operator=(const FCam::Shot &);
            const Shot &operator=(const Shot &);

        };
    
    }
}

#endif
