#ifndef FCAM_HISTOGRAM_H
#define FCAM_HISTOGRAM_H

#include <vector>

/*! \file
 * The Histogram and HistogramConfig classes
 */  

namespace FCam {
    /** The configuration of the histogram generator. */
    class HistogramConfig {         
      public:
        /** The default constructor disables the histogram
         * generator. */
        HistogramConfig() : buckets(64), enabled(false) {}
      
        /** Rectangle specifying the region over which a histogram is
         * computed. */
        Rect region;

        /** The requested number of buckets in the histogram. The N900
         * implementation ignores this request and gives you a
         * 64-bucket histogram. */
        unsigned buckets;

        /** Whether or not a histogram should be generated at all */
        bool enabled;

        /** Compare two requested configurations to see if they would
         * return the same data. */
        bool operator==(const HistogramConfig &other) const {
            if (enabled != other.enabled) return false;
            if (buckets != other.buckets) return false;
            if (region != other.region) return false;
            return true;
        }

        /** Compare two requested configurations to see if they would
         * return the same data. */
        bool operator!=(const HistogramConfig &other) const {
            return !((*this) == other);
        }
    };


    /** A histogram returned by the histogram generator. Before you
     * dereference the data, check if valid is true. Even if you
     * requested a histogram, you're not guaranteed to get one back in
     * all cases. */
    class Histogram {
        unsigned _buckets, _channels;
        Rect _region;
        std::vector<unsigned> _data;

      public:

        /** The default constructor is an invalid histogram storing no
         * data. */
        Histogram(): _buckets(0), _channels(0), _region(0, 0, 0, 0) {
        }

        /** Make a new empty histogram of the given number of buckets and channels. */
        Histogram(unsigned buckets, unsigned channels, Rect region) :
            _buckets(buckets), _channels(channels), _region(region) {
            _data.resize(buckets*channels);
        }

        /** Sample the histogram at a particular bucket in a
         * particular image channel. The absolute number should not be
         * relied on, because the histogram is computed over the
         * bayer-mosaiced raw sensor data, which may be a different
         * resolution to the actual output image if the imaging pipe
         * is doing some resizing. If color histograms are available,
         * the order of the channels is RGB, which will be in the
         * sensor's raw color space. Despite there being twice as many
         * green samples on the raw sensor, the green channel is
         * normalized to have roughly the same total count as red and
         * blue. */
        unsigned operator()(int b, int c) const {
            return _data[b*_channels + c];
        }

        /** Acquire a reference to a particular histogram
         * entry. Useful for creating your own histograms. */
        unsigned &operator()(int b, int c) {
            return _data[b*_channels + c];
        }

        /** Sample the histogram at a particular bucket, summed over
         * all channels. */
        unsigned operator()(int b) const {
            unsigned result = 0;
            for (size_t c = 0; c < _channels; c++)
                result += (*this)(b, c);
            return result;
        }

        /** Is it safe to dereference data and/or call operator(). */
        bool valid() const {
            return _data.size() != 0;
        }

        /** Access to the raw histogram data. Stored similarly to an
         * image: first bucket, then color channel. */
        unsigned *data() {return &_data[0];}  

        /** How many buckets does this histogram have. */
        unsigned buckets() const {return _buckets;}

        /** How many channels does this histogram have. */
        unsigned channels() const {return _channels;}

        /** What region of the image was this histogram computed over. */
        Rect region() const {return _region;}
    };
   
}

#endif
