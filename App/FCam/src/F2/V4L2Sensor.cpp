#include <asm/types.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <linux/capability.h>

#include <pthread.h>
#include <poll.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>

#include <errno.h>
#include <malloc.h>
#include <linux/videodev2.h>

#include "../Debug.h"
#include "FCam/Event.h"
#include "V4L2Sensor.h"
#warning make sure to point isp_user to the right place before long!
#include "/elphel/git/linux-omap-2.6/drivers/media/video/isp/isp_user.h"
//#include "/elphel/git/linux-omap-2.6/arch/arm/plat-omap/include/mach/isp_user.h"
//#include "linux/isp_user.h"

namespace FCam { namespace F2 {

    V4L2Sensor *V4L2Sensor::instance(std::string fname) {
        std::map<std::string, V4L2Sensor *>::iterator i;
        i = instances_.find(fname);
        if (i == instances_.end()) {
            instances_[fname] = new V4L2Sensor(fname);            
        }

        return instances_[fname];
    };

    V4L2Sensor::V4L2Sensor(std::string fname) : state(CLOSED), filename(fname) {
        
    }

    std::map<std::string, V4L2Sensor *> V4L2Sensor::instances_;

    void V4L2Sensor::open() {
        if (state != CLOSED) {
            printf("Sensor is already open!\n");
            return;
        }

        fd = ::open(filename.c_str(), O_RDWR | O_NONBLOCK, 0);
    
        if (fd < 0) {
            error(Event::DriverError,"V4L2Sensor: Error opening device %s: %s", filename.c_str(), strerror(errno));
            return;
        }
    
        state = IDLE;
    }

    void V4L2Sensor::close() {
        switch(state) {
        case STREAMING:
            stopStreaming();
        case IDLE:
            ::close(fd);
        case CLOSED:
            break;
        }
        state = CLOSED;
    }

    int V4L2Sensor::getFD() {
        if (state == CLOSED) {
            return -1;
        }
        return fd;
    }

    void V4L2Sensor::startStreaming(Mode m, 
                                    const HistogramConfig &histogram,
                                    const SharpnessMapConfig &sharpness) {
        
        if (state != IDLE) {
            printf("Can only initiate streaming if sensor is idle\n");
            return;
        }

        struct v4l2_format fmt;
        
        memset(&fmt, 0, sizeof(struct v4l2_format));
        
        fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = m.width;
        fmt.fmt.pix.height      = m.height;
        if (m.type == UYVY) {
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
        } else if (m.type == RAW) {
            fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SGRBG10;
        } else {
            error(Event::InternalError, "V4L2Sensor: Unknown image format requested");
            return;
        }
        fmt.fmt.pix.field       = V4L2_FIELD_NONE;

        // Request format
        if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
            error(Event::DriverError,"VIDIOC_S_FMT: %s", strerror(errno));
            return;
        }
    
        currentMode.width = fmt.fmt.pix.width;
        currentMode.height = fmt.fmt.pix.height;
        currentMode.type = (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_UYVY) ? UYVY : RAW;

        struct v4l2_requestbuffers req;    
        memset(&req, 0, sizeof(req));
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        req.count  = 8;

        if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
            error(Event::DriverError,"VIDIOC_REQBUFS: %s", strerror(errno));
            return;
        } 

        buffers.resize(req.count);

        for (size_t i = 0; i < buffers.size(); i++) {
            v4l2_buffer buf;
            memset(&buf, 0, sizeof(v4l2_buffer));
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index  = i;

            if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
                error(Event::DriverError,"VIDIOC_QUERYBUF: %s", strerror(errno));
                return;
            }
        
            buffers[i].index = i;
            buffers[i].length = buf.length;
            buffers[i].data = 
                (unsigned char *)mmap(NULL, buffers[i].length, PROT_READ | PROT_WRITE,
                                      MAP_SHARED, fd, buf.m.offset);
        
            if (buffers[i].data == MAP_FAILED) {
                error(Event::InternalError, "V4L2Sensor: mmap failed: %s", strerror(errno));
                return;
            }
        }   

        for (size_t i = 0; i < buffers.size(); i++) {
            releaseFrame(&buffers[i]);
        }

        // set the starting parameters
        setHistogramConfig(histogram);
        setSharpnessMapConfig(sharpness);

        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
            error(Event::DriverError,"VIDIOC_STREAMON: %s", strerror(errno));
            return;
        }
        
        dprintf(DBG_MAJOR, "Sensor now streaming\n");
        state = STREAMING;
    }


    void V4L2Sensor::stopStreaming() {
        if (state != STREAMING) {
            printf("Camera is already not streaming!\n");
            return;
        }

        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
            error(Event::DriverError,"VIDIOC_STREAMOFF: %s", strerror(errno));
            return;
        }

        for (size_t i = 0; i < buffers.size(); i++) {
            if (munmap(buffers[i].data, buffers[i].length)) {
                error(Event::InternalError, "V4L2Sensor: munmap failed: %s", strerror(errno));
            }
        }

        state = IDLE;
    }



    V4L2Sensor::V4L2Frame *V4L2Sensor::acquireFrame(bool blocking) {
        if (state != STREAMING) {
            printf("Can't acquire a frame when not streaming!\n");
            return NULL;
        }

        //printf("Acquiring frame...\n"); fflush(stdout);
        
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(v4l2_buffer));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        
        if (blocking) {
            struct pollfd p = {fd, POLLIN, 0};
            poll(&p, 1, -1);
            if (!(p.revents & POLLIN)) {
                error(Event::DriverError,"Poll returned without data being available: %s", strerror(errno));
                return NULL;
            }
        }    
        
        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
            if (errno == EAGAIN && !blocking) {
                return NULL;
            }
            
            error(Event::DriverError,"VIDIOC_DQBUF: %s", strerror(errno));
            return NULL;
        }


        buffers[buf.index].processingDoneTime = Time(buf.timestamp);
        return &(buffers[buf.index]);
    }



    Histogram V4L2Sensor::getHistogram(Time t, const HistogramConfig &conf) {

        //printf("Buffer timestamp: %d %d\n", t.s(), t.us());
        if (!conf.enabled) {
            return Histogram();
        }

        // grab the histogram data for this frame
        struct isp_hist_data hist_data;
        unsigned buf[64 * 4];  // Hardcoded for OMAP3 ISP histogram (64 buckets, 4 channels)
        Time h;
        int tries = 3;
        while (--tries > 0) {
            hist_data.hist_statistics_buf = buf;
            hist_data.update = REQUEST_STATISTICS;
            
            // For now we assume acquire_frame is being called quickly
            // enough that only the newest frame is relevant
            hist_data.frame_number = NEWEST_FRAME;
            hist_data.curr_frame = 0;
            hist_data.config_counter = 0;
            hist_data.ts.tv_sec = 0;
            hist_data.ts.tv_usec = 0;
            
            if (ioctl(fd, VIDIOC_PRIVATE_ISP_HIST_REQ, &hist_data)) {
                error(Event::DriverError, "V4L2Sensor::getHistogram: VIDIOC_PRIVATE_ISP_HIST_REQ: %s\n",strerror(errno));
                return Histogram();
            }          
        
            // TODO: Deal with timestamps that are too early or too late
            
            h = Time(hist_data.ts);        
            
            if ((t - h) < 6000) {
                break;
            }
            usleep(10000);
        }
        if (tries != 2) dprintf(DBG_MINOR, "V4L2Sensor::getHistogram: Waited for the histogram %d times\n", 2-tries);
        if (!tries) {
            error(Event::DriverError, "Histogram cannot be found\n");
            return Histogram();
        }
        
         while ((t-h) < -4000) {
             // we got the wrong histogram!
             if (hist_data.frame_number == 0) hist_data.frame_number = 4095;
             else hist_data.frame_number--;

             if (ioctl(fd, VIDIOC_PRIVATE_ISP_HIST_REQ, &hist_data)) {
                error(Event::DriverError, "V4L2Sensor::getHistogram: VIDIOC_PRIVATE_ISP_HIST_REQ: %s\n", strerror(errno));
                return Histogram();
             }          

             h = Time(hist_data.ts);
         }
        Histogram hist(64, 3, conf.region);
        for (int i = 0; i < 64; i++) {
            hist(i, 0) = buf[64 + i];  // r
            hist(i, 1) = buf[i];       // g
            hist(i, 2) = buf[128 + i]; // b
        }
        return hist;
    }

    SharpnessMap V4L2Sensor::getSharpnessMap(Time t, const SharpnessMapConfig &conf) {    
        if (!conf.enabled) {
            return SharpnessMap();
        }

        // grab the sharpness map for this frame        
        struct isp_af_data af_data;
        af_data.frame_number = NEWEST_FRAME;
        af_data.update = REQUEST_STATISTICS;
        af_data.curr_frame = 0;
        af_data.config_counter = 0;
        af_data.xtrastats.ts.tv_sec = 0;
        af_data.xtrastats.ts.tv_usec = 0;
        unsigned buf[16*12*12]; // Hardcoded for OMAP3 ISP size
        af_data.af_statistics_buf = buf;
        
        if (ioctl(fd, VIDIOC_PRIVATE_ISP_AF_REQ, &af_data)) {
            error(Event::DriverError, "V4L2Sensor::getSharpnessMap: VIDIOC_PRIVATE_ISP_AF_REQ: %s\n", strerror(errno) );
            return SharpnessMap();
        }          

        Time s(af_data.xtrastats.ts);        
        if ((t - s) > 6000) {
            warning(Event::DriverError, "Missing sharpness: (%d)\n", t-s );
        }

        while ((t-s) < -2000) {
            // we got the wrong sharpness map
            if (af_data.frame_number == 0) af_data.frame_number = 4095;
            else af_data.frame_number--;

            if (ioctl(fd, VIDIOC_PRIVATE_ISP_AF_REQ, &af_data)) {
                error(Event::DriverError, "V4L2Sensor::getSharpnessMap: VIDIOC_PRIVATE_ISP_AF_REQ: %s\n", strerror(errno) );
                return SharpnessMap();
            }          
            s = Time(af_data.xtrastats.ts);
        }
        SharpnessMap m(Size(16,12), 3);
        unsigned *bufPtr = &buf[0];
        for (int y = 0; y < m.size().height; y++) {
            for (int x = 0; x < m.size().width; x++) {
                m(x, y, 0) = bufPtr[1];
                m(x, y, 1) = bufPtr[5];
                m(x, y, 2) = bufPtr[9];
                bufPtr += 12;
            }
        }

        return m;
    }

    void V4L2Sensor::setHistogramConfig(const HistogramConfig &histogram) {       
        if (!histogram.enabled) return;

         // get the output size from the ccdc
         isp_pipeline_stats pstats;
         if (ioctl(fd, VIDIOC_PRIVATE_ISP_PIPELINE_STATS_REQ, &pstats) < 0) {
            error(Event::DriverError, "V4L2Sensor::setHistogramConfig: VIDIOC_PRIVATE_ISP_PIPELINE_STATS_REQ: %s\n", strerror(errno));
             return;
         }
                
         dprintf(4, "CCDC output: %d x %d\n", pstats.ccdc_out_w, pstats.ccdc_out_h);
         dprintf(4, "PRV  output: %d x %d\n", pstats.prv_out_w, pstats.prv_out_h);
         dprintf(4, "RSZ  input:  %d x %d + %d, %d\n",
                pstats.rsz_in_w, pstats.rsz_in_h,
                pstats.rsz_in_x, pstats.rsz_in_y);
         dprintf(4, "RSZ  output: %d x %d\n", pstats.rsz_out_w, pstats.rsz_out_h);
        
        struct isp_hist_config hist_cfg;                       
        hist_cfg.enable = 1;
        hist_cfg.source = HIST_SOURCE_CCDC;
        hist_cfg.input_bit_width = 10;
        hist_cfg.num_acc_frames = 1;
        hist_cfg.hist_bins = HIST_BINS_64;
        hist_cfg.cfa = HIST_CFA_BAYER;
        // set the gains to slightly above 1 in 3Q5 format, in
        // order to use the full range in the histogram. Without
        // this, bucket #60 is saturated pixels, and 61-64 are
        // unused.
        hist_cfg.wg[0] = 35;
        hist_cfg.wg[1] = 35;
        hist_cfg.wg[2] = 35;
        hist_cfg.wg[3] = 35;
        hist_cfg.num_regions = 1;
        
        // set up its width and height
        unsigned x = ((unsigned)histogram.region.x * pstats.ccdc_out_w) / currentMode.width;
        unsigned y = ((unsigned)histogram.region.y * pstats.ccdc_out_h) / currentMode.height;
        unsigned w = ((unsigned)histogram.region.width * pstats.ccdc_out_w) / currentMode.width;
        unsigned h = ((unsigned)histogram.region.height * pstats.ccdc_out_h) / currentMode.height;
        if (x > pstats.ccdc_out_w) x = pstats.ccdc_out_w-1;
        if (y > pstats.ccdc_out_h) y = pstats.ccdc_out_h-1;
        if (w > pstats.ccdc_out_w) w = pstats.ccdc_out_w-x;
        if (h > pstats.ccdc_out_h) h = pstats.ccdc_out_h-y;
        hist_cfg.reg_hor[0] = (x << 16) | w;
        hist_cfg.reg_ver[0] = (y << 16) | h;
        dprintf(4, "Histogram size: %d x %d + %d, %d\n", w, h, x, y);
        
        dprintf(DBG_MINOR, "Enabling histogram generator\n");
        // enable the histogram generator
        if (ioctl(fd, VIDIOC_PRIVATE_ISP_HIST_CFG, &hist_cfg)) {
            error(Event::DriverError, "VIDIOC_PRIVATE_ISP_HIST_CFG: %s", strerror(errno));
            return;
        }

        currentHistogram = histogram;
        currentHistogram.buckets = 64;
    }

    void V4L2Sensor::setSharpnessMapConfig(const SharpnessMapConfig &sharpness) {       
        if (!sharpness.enabled) return;

        // Ignore the requested size and use 16x12
        Size size = Size(16, 12);

        // get the output size from the ccdc
        isp_pipeline_stats pstats;
        if (ioctl(fd, VIDIOC_PRIVATE_ISP_PIPELINE_STATS_REQ, &pstats) < 0) {
            error(Event::DriverError, "VIDIOC_PRIVATE_ISP_PIPELINE_STATS_REQ: %s", strerror(errno));
            return;
        }
   
        struct af_configuration af_config;
        
        af_config.alaw_enable = H3A_AF_ALAW_DISABLE;
        af_config.hmf_config.enable = H3A_AF_HMF_ENABLE;
        af_config.hmf_config.threshold = 10;
        af_config.rgb_pos = RG_GB_BAYER;
        af_config.iir_config.hz_start_pos = 0;
        
        // The IIR coefficients are as follows (yay reverse-engineering!)
        
        // The format is S6Q5 fixed point. A positive value of x
        // should be written as 32*x. A negatives value of x
        // should be written as 4096 - 32*x
        
        // 0: global gain? not sure.
        // 1-2: IIR taps on the first biquad
        // 3-5: FIR taps on the first biquad
        // 6-7: IIR taps on the second biquad
        // 8-10: FIR taps on the second biquad            
        
        // A high pass filter aimed at ~8 pixel frequencies and above
        af_config.iir_config.coeff_set0[0] = 32; // gain of 1
        
        af_config.iir_config.coeff_set0[1] = 4096-27;
        af_config.iir_config.coeff_set0[2] = 6;
        
        af_config.iir_config.coeff_set0[3] = 16;
        af_config.iir_config.coeff_set0[4] = 4096-32;
        af_config.iir_config.coeff_set0[5] = 16;
        
        af_config.iir_config.coeff_set0[6] = 0;
        af_config.iir_config.coeff_set0[7] = 0;
        
        af_config.iir_config.coeff_set0[8] = 32;
        af_config.iir_config.coeff_set0[9] = 0;
        af_config.iir_config.coeff_set0[10] = 0;
        
        
        // A high pass filter aimed at ~4 pixel frequencies and above
        af_config.iir_config.coeff_set1[0] = 32; // gain of 1
        
        af_config.iir_config.coeff_set1[1] = 0;
        af_config.iir_config.coeff_set1[2] = 0;
        
        af_config.iir_config.coeff_set1[3] = 16;
        af_config.iir_config.coeff_set1[4] = 4096-32;
        af_config.iir_config.coeff_set1[5] = 16;
        
        af_config.iir_config.coeff_set1[6] = 0;
        af_config.iir_config.coeff_set1[7] = 0;
        
        af_config.iir_config.coeff_set1[8] = 32;
        af_config.iir_config.coeff_set1[9] = 0;
        af_config.iir_config.coeff_set1[10] = 0;
        
        af_config.mode = ACCUMULATOR_SUMMED;
        af_config.af_config = H3A_AF_CFG_ENABLE;
        int paxWidth = ((pstats.ccdc_out_w-4) / (2*size.width))*2;
        int paxHeight = ((pstats.ccdc_out_h-4) / (2*size.height))*2;

        if (paxWidth > 256) {
            error(Event::InternalError, "AF paxels are too wide. Use a higher resolution sharpness map\n");
            return;
        }
        if (paxHeight > 256) {
            error(Event::InternalError, "AF paxels are too tall. Use a higher resolution sharpness map\n");
            return;
        }
        if (paxWidth < 16) {            
            error(Event::InternalError, "AF paxels are too narrow. Use a lower resolution sharpness map\n");
            return;
        }
        if (paxHeight < 2) {
            error(Event::InternalError, "AF paxels are too short. Use a lower resolution sharpness map\n");
            return;
        }
        
        dprintf(4, "V4L2Sensor::setSharpnessMapConfig: Using %d x %d paxels for af\n", paxWidth, paxHeight);
        af_config.paxel_config.width = (paxWidth-2)/2;
        af_config.paxel_config.height = (paxHeight-2)/2;
        af_config.paxel_config.hz_start = (pstats.ccdc_out_w - size.width * paxWidth)/2;
        af_config.paxel_config.vt_start = (pstats.ccdc_out_h - size.height * paxHeight)/2;
        af_config.paxel_config.hz_cnt = size.width-1;
        af_config.paxel_config.vt_cnt = size.height-1;
        af_config.paxel_config.line_incr = 0;            
        
        dprintf(DBG_MINOR, "Enabling sharpness detector\n");
        if (ioctl(fd, VIDIOC_PRIVATE_ISP_AF_CFG, &af_config)) {
            error(Event::DriverError, "VIDIOC_PRIVATE_ISP_AF_CFG: %s", strerror(errno));
            return;
        }

        currentSharpness = sharpness;
    }

    void V4L2Sensor::releaseFrame(V4L2Frame *frame) {
        //printf("Releasing frame %d\n", frame->index); fflush(stdout);

        //usleep(1000);
        
        // requeue the buffer
        v4l2_buffer buf;
        memset(&buf, 0, sizeof(v4l2_buffer));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = frame->index;
        
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            error(Event::DriverError, "VIDIOC_QBUF: %s", strerror(errno));
            return;
        }
    }
   
    void V4L2Sensor::setControl(unsigned int id, int value) {
        if (state == CLOSED) return;
        v4l2_control ctrl;
        ctrl.id = id;
        ctrl.value = value;
        if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
            // TODO: Better error reporting for all the get/set
            error(Event::DriverError, "VIDIOC_S_CTRL: %s", strerror(errno));
            return;
        }
    }

    int V4L2Sensor::getControl(unsigned int id) {
        if (state == CLOSED) return -1;
        v4l2_control ctrl;
        ctrl.id = id;
        if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) < 0) {
            error(Event::DriverError, "VIDIOC_G_CTRL: %s", strerror(errno));
            return -1;
        }

        return ctrl.value;
    }

    void V4L2Sensor::setExposure(int e) {
        if (state == CLOSED) return;

        struct v4l2_control ctrl;
        ctrl.id = V4L2_CID_EXPOSURE;
        ctrl.value = e;
        if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
            error(Event::DriverError,"VIDIOC_S_CTRL: %s", strerror(errno));
            return;
        }       
        
    }

    int V4L2Sensor::getExposure() {
        if (state == CLOSED) return -1;        

        struct v4l2_control ctrl;
        ctrl.id = V4L2_CID_EXPOSURE;
        
        if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) < 0) {
            error(Event::DriverError,"VIDIOC_G_CTRL: %s", strerror(errno));
            return -1;
        }       
        
        return ctrl.value;
    }

#define V4L2_CID_FRAME_TIME (V4L2_CTRL_CLASS_CAMERA | 0x10ff)

    void V4L2Sensor::setFrameTime(int e) {
        if (state == CLOSED) return;
        
        struct v4l2_control ctrl;
        ctrl.id = V4L2_CID_FRAME_TIME;
        ctrl.value = e;
        if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
            error(Event::DriverError,"VIDIOC_S_CTRL: %s", strerror(errno));
            return;
        }       
    }

    int V4L2Sensor::getFrameTime() {
        if (state == CLOSED) return -1;        

        struct v4l2_control ctrl;
        ctrl.id = V4L2_CID_FRAME_TIME;
        
        if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) < 0) {
            error(Event::DriverError,"VIDIOC_G_CTRL: %s", strerror(errno));
            return -1;
        }       
        
        return ctrl.value;
    }

    void V4L2Sensor::setGain(float g) {
        if (state == CLOSED) return;

        unsigned int gain;
        struct v4l2_control ctrl;
        
        gain = (int)(g * 32.0 + 0.5);
        
        ctrl.id = V4L2_CID_GAIN;
        ctrl.value = gain;
        if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
            error(Event::DriverError,"VIDIOC_S_CTRL: %s", strerror(errno));
            return;
        }              
    }

    float V4L2Sensor::getGain() {
        if (state == CLOSED) return -1.0f;

        struct v4l2_control ctrl;
        
        ctrl.id = V4L2_CID_GAIN;
        if (ioctl(fd, VIDIOC_G_CTRL, &ctrl) < 0) {
            error(Event::DriverError,"VIDIOC_G_CTRL: %s", strerror(errno));
            return -1.0f;
        }       
        
        return ctrl.value / 32.0f;    
    }
}}


