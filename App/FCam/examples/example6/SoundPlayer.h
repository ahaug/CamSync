#ifndef FCAM_BEEPER_H
#define FCAM_BEEPER_H

/** \file */

#include <string>

#include <FCam/FCam.h>
#include <FCam/Action.h>
#include <FCam/Device.h>

// PulseAudio headers
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#define BUFSIZE 1024

/*
 * A synchronized beeper example. As a device,
 * it inherits from FCam::Device, and declares
 * nested classes that inherit from CopyableAction
 */
class SoundPlayer : public FCam::Device {
    
public:
    
    SoundPlayer();
    ~SoundPlayer();
    
    /*
     * An action representing the playback of a .WAV file.
     */
    class SoundAction : public FCam::CopyableAction<SoundAction> {
    public:

        /* Constructors and destructor */
        ~SoundAction();
        SoundAction(SoundPlayer * b);
        SoundAction(SoundPlayer * b, int time);
        SoundAction(const SoundAction &);
        
        /* Implementation of doAction() as required */
        void doAction();
        
        /* Load the specified file into buffer and prepares playback */
        void setWavFile(const char * filename);
        
        /* Return the underlying device */
        SoundPlayer * getPlayer() const { return player; }

    protected:
        
        SoundPlayer * player;
        unsigned int *refCount;
        std::string filename;
        unsigned char * buffer; // reference-counted dump of wave file.
        int size;
    };
    
    /* Normally, this is where a device would add metadata tags to a
     * just-created frame , based on the timestamps in the
     * Frame. However, we don't have anything useful to add here, so
     * tagFrame does nothing. */
    void tagFrame(FCam::Frame) {}
    
    /* Play a buffer */
    void playBuffer(unsigned char * buffer, size_t size);
    
    /* Returns latency in microseconds */
    int getLatency();
    
protected:
    pa_simple * connection;
    
};

#endif
