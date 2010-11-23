#include "SoundPlayer.h"

/** \file */

/***************************************************************/
/* SoundPlayer implementation                                  */
/***************************************************************/

/* SoundPlayer constructor */
SoundPlayer::SoundPlayer() {
    // Create a new playback
    int error;
    static const pa_sample_spec ss = {PA_SAMPLE_S16LE, 24000, 1};
    if (!(connection = pa_simple_new(NULL, NULL, PA_STREAM_PLAYBACK, NULL,
                                     "playback", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n",
                pa_strerror(error));
    }
}

/* SoundPlayer destructor */
SoundPlayer::~SoundPlayer() {
    if (connection)
        pa_simple_free(connection);
}

/* Play a buffer */
void SoundPlayer::playBuffer(unsigned char * b, size_t s) {
    if (connection) {
        int error;
        // Play buffer
        if (pa_simple_write(connection, b, s, &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n",
                    pa_strerror(error));
            return;
        }
    }
}

int SoundPlayer::getLatency() {
    if (connection) {
        int error;
        pa_usec_t ret = pa_simple_get_latency(connection, &error);
        return (int)ret;
    } else return 0;
}

/***************************************************************/
/* SoundPlayer::SoundAction implementation                     */
/***************************************************************/

/* SoundAction constructors */
SoundPlayer::SoundAction::SoundAction(SoundPlayer * a) {
    player = a;
    time = 0;
    latency = a ? a->getLatency() : 0;
    buffer = NULL;
}

SoundPlayer::SoundAction::SoundAction(SoundPlayer * a, int t) {
    player = a;
    time = t;
    latency = a ? a->getLatency() : 0;
    buffer = NULL;
}

SoundPlayer::SoundAction::SoundAction(const SoundPlayer::SoundAction & b) {
    // Copy fields from the target.
    time = b.time;
    latency = b.latency;
    player = b.getPlayer();
    if (b.buffer) {
        // Increment reference counter for the buffer.
        refCount = b.refCount;
        buffer = b.buffer;
        size = b.size;
        *refCount++;
    } else {
        buffer = NULL;
    }  
}

/* SoundAction destructor */
SoundPlayer::SoundAction::~SoundAction() {
    if (buffer) {
        *refCount--; // Decrement the reference counter. 
        if (*refCount == 0) { // Deallocate if reference counter is 0.
            delete refCount;
            delete[] buffer;
        }
    }
}

void SoundPlayer::SoundAction::setWavFile(const char * f) {

    // Delete existing connection
    if (buffer) {
        *refCount--;
        if (*refCount == 0) {
            delete buffer;
            delete refCount;
        }
        buffer = NULL;
    }

    filename = std::string(f);
    int fd;
  
    // Check that the file can be opened.
    if ((fd = open(f, O_RDONLY)) < 0) {
        fprintf(stderr, __FILE__": open() failed: %s\n", strerror(errno));
        return;
    } 

    // Find out how long the file is
    struct stat stat_buf;
    fstat(fd, &stat_buf);
    ssize_t capacity = stat_buf.st_size;

    // Read the file and store into a buffer
    buffer = new unsigned char[capacity];
    size = 0;
    while (size < capacity) {
        ssize_t r = read(fd, buffer + size, 4096);
        if (r == 0) break; // EOF
        if (r < 0) {
            fprintf(stderr, __FILE__": read() failed: %s\n", strerror(errno));
        }
        size += r;
    }

    close(fd);

    // Make a reference count for the buffer
    refCount = new unsigned int;
    *refCount = 1;
}

/* Perform the required action */
void SoundPlayer::SoundAction::doAction() {
    if (buffer) {
        player->playBuffer(buffer, size);
    }
}



