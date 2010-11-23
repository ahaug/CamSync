/*
 * drivers/media/video/mt9p031.h
 *
 * Register definitions for the MT9P031 camera sensor.
 *
 * Author:
 *      Eino-Ville Talvala <talvala@stanford.edu>
 * Based on mt9p012.h by
 * 	Sameer Venkatraman <sameerv@ti.com>
 * 	Martinez Leonides
 *
 *
 * Copyright (C) 2008 Texas Instruments.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef MT9P031_H
#define MT9P031_H

/* Possible I2C addresses for MT9PO31, depends on state of S_ADDR pin */

#define MT9P031_I2C_ADDR_LOW		0x48 // Shifted right by 1 from datasheet addr
#define MT9P031_I2C_ADDR_HIGH		0x5D // Shifted right by 1 from datasheet addr

/* Pixel array size limits.
 * The active pixel array exists from (0,0)-(MT9P031_ACTIVE_WIDTH-1, MT9P031_ACTIVE_HEIGHT-1)
 * The total pixel array, including black pixels, exists from (MT9P031_MIN_COL, MT9P031_MIN_ROW)-
 * (MT9P031_MAX_COL, MT9P031_MAX_ROW) 
 */

#define MT9P031_ACTIVE_WIDTH	    2592  //v
#define MT9P031_ACTIVE_HEIGHT	    1944  //v
#define MT9P031_ARRAY_WIDTH         2752  // Includes black pixels  
#define MT9P031_ARRAY_HEIGHT        2004  // Includes black pixels  
#define MT9P031_MIN_COL            -16   // Includes black pixels
#define MT9P031_MIN_ROW            -54   // Includes black pixels
#define MT9P031_MAX_COL            (MT9P031_MIN_COL + MT9P031_ARRAY_WIDTH)
#define MT9P031_MAX_ROW            (MT9P031_MIN_ROW + MT9P031_ARRAY_HEIGHT)
#define MT9P031_IMAGE_WIDTH_MIN	    2  //v
#define MT9P031_IMAGE_HEIGHT_MIN    2  //v
#define MT9P031_IMAGE_WIDTH_DEF	    640  //v
#define MT9P031_IMAGE_HEIGHT_DEF    480  //v

/* Custom V4L2 controls */

/* Per-frame settings: */

/* -Exposure, gain already defined by V4L2 */
/* -Set binning and skipping modes */
#define MT9P031_CID_ROW_BINNING   (V4L2_CID_PRIVATE_BASE+9)
#define MT9P031_CID_ROW_SKIPPING  (V4L2_CID_PRIVATE_BASE+10)
#define MT9P031_CID_COL_BINNING   (V4L2_CID_PRIVATE_BASE+11)
#define MT9P031_CID_COL_SKIPPING  (V4L2_CID_PRIVATE_BASE+12)
#define MT9P031_CID_SKIPBIN       (V4L2_CID_PRIVATE_BASE+13)
/* -Set ROI corner */
#define MT9P031_CID_ROI_X         (V4L2_CID_PRIVATE_BASE+14)
#define MT9P031_CID_ROI_Y         (V4L2_CID_PRIVATE_BASE+15)
/* Set frame time (adds VBLANK as needed) */
#define MT9P031_CID_FRAME_TIME    (V4L2_CTRL_CLASS_CAMERA | 0x10ff)
/* -Set no processing mode */
#define MT9P031_CID_NO_PROCESSING (V4L2_CID_PRIVATE_BASE+16)

/* Parameter set use and manipulation */
#define MT9P031_CID_DIRECT_MODE  (V4L2_CID_PRIVATE_BASE+17)
#define MT9P031_CID_ATOMIC_MODE  (V4L2_CID_PRIVATE_BASE+18)
#define MT9P031_CID_WRITE_PARAMS  (V4L2_CID_PRIVATE_BASE+19)
#define MT9P031_CID_READ_PARAMS   (V4L2_CID_PRIVATE_BASE+20)
#define MT9P031_CID_ACTIVE_PARAM_COUNT  (V4L2_CID_PRIVATE_BASE+21)

/* Per-stream mode settings */
#define MT9P031_CID_STREAMING_MODE (V4L2_CID_PRIVATE_BASE+22)

/* Current vector index in use. Needed by omap34xxcam hack. */
#define MT9P031_CID_VECTOR_INDEX (V4L2_CID_PRIVATE_BASE+23)

/* Daemon IOCTLs */

#define MT9P031_CID_WAIT_HSVS_1     (V4L2_CID_PRIVATE_BASE+24)
#define MT9P031_CID_WAIT_HSVS_2     (V4L2_CID_PRIVATE_BASE+25)

/* Magic CIDs for development */
#define MT9P031_CID_MAGIC         (V4L2_CID_PRIVATE_BASE+26)
#define MT9P031_CID_MAGIC2        (V4L2_CID_PRIVATE_BASE+27)

/* Special values for specific controls */

/* ROI_X/ROI_Y automatic center placement */
#define MT9P031_ROI_AUTO_X MT9P031_MIN_COL-1
#define MT9P031_ROI_AUTO_Y MT9P031_MIN_ROW-1
/* Readback of current sensor settings into scratch */
#define MT9P031_READ_CURRENT_PARAMS -1


enum mt9p031_row_bin {
  MT9P031_ROW_1X_BIN = 0x00,
  MT9P031_ROW_2X_BIN = 0x01,
  MT9P031_ROW_3X_BIN = 0x02,
  MT9P031_ROW_4X_BIN = 0x03,
};

enum mt9p031_row_skip {
  MT9P031_ROW_1X_SKIP = 0,
  MT9P031_ROW_2X_SKIP,
  MT9P031_ROW_3X_SKIP,
  MT9P031_ROW_4X_SKIP,
  MT9P031_ROW_5X_SKIP,
  MT9P031_ROW_6X_SKIP,
  MT9P031_ROW_7X_SKIP,
  MT9P031_ROW_8X_SKIP
};

enum mt9p031_col_bin {
  MT9P031_COL_1X_BIN = 0x00,
  MT9P031_COL_2X_BIN = 0x01,
  MT9P031_COL_4X_BIN = 0x03,
};

enum mt9p031_col_skip {
  MT9P031_COL_1X_SKIP = 0,
  MT9P031_COL_2X_SKIP,
  MT9P031_COL_3X_SKIP,
  MT9P031_COL_4X_SKIP,
  MT9P031_COL_5X_SKIP,
  MT9P031_COL_6X_SKIP,
  MT9P031_COL_7X_SKIP,
};


/**
 * struct mt9p031_sensor_parameters - Stores commonly changed sensor
 *   settings for easy comparison when updating frames
 */
#define MAX_PARAMETER_VECTOR 8
struct mt9p031_sensor_parameters {
        int exposure;    // microseconds
        int gain;        // linear
        int frame_time;  // microseconds

	enum mt9p031_row_skip row_skip;
	enum mt9p031_row_bin  row_bin;
	enum mt9p031_col_skip col_skip;
	enum mt9p031_col_bin  col_bin;

        int roi_x;       // (0,0) is the top-left of the active pixel area. 
        int roi_y;

	int hblank_extra;
	//int vblank_extra;
  
        int no_processing; /* For capturing fully raw data (no black level compensation, etc) */
};


#endif /* ifndef MT9P031_H */
