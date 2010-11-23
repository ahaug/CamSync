#ifndef VISUALIZATION_WIDGET_H
#define VISUALIZATION_WIDGET_H

#include <QWidget>
#include <QPaintEvent>

#include "ImageItem.h"
#include "Viewfinder.h"

#include <FCam/N900.h>

/** A widget thath draws grid lines over the viewfinder.
  * Useful for creating rule of thirds guides, levels, etc. */

class GridWidget : public QWidget {
    Q_OBJECT
public:
    GridWidget(unsigned int numRows, unsigned int numCols, QWidget * parent);
protected:
    // The number of rows and columns in the grid.
    unsigned int rows, cols;
    // Override the paint event to draw rows-1 horizontal lines
    // and cols-1 vertical lines evenly distributed throughout this
    // widgets area.
    virtual void paintEvent(QPaintEvent * event);
    // Paint the grid with the given pen. Called as a subroutine
    // to paintEvent to manage the black outlines.
    void paintGrid(QPen pen);
};


/** A widget that draws a white histogram with a black
 * boundary. Useful for visualizations. */
class HistogramWidget : public QWidget {
    Q_OBJECT
public:
    HistogramWidget(QWidget * parent);
    void setHistogram(FCam::Histogram hist);
protected:
    // The histogram to draw.
    FCam::Histogram histogram;
    // Override the paint event to draw the histogram to fit within
    // this widget's area.
    virtual void paintEvent(QPaintEvent * event);       
};


/** The visualization widget sits on top of the viewfinder, and
 * contains the various visualizations. Every time a new viewfinder
 * frame is available from the camera thread, it uses the UserDefaults
 * instance to determine which ones should be drawn and where. */
class VisualizationWidget : public QWidget {
    Q_OBJECT
public:
    VisualizationWidget(Viewfinder * vf, QWidget * parent);
protected:
    // A pointer to the viewfinder we live in, so we can query it to
    // see where it's safe to place visualizations. Otherwise we risk
    // fighting with whatever adjustment widgets are visible.
    Viewfinder * viewfinder;
    
    // All the possible visualizations. Add your own here!
    HistogramWidget * intensityHistogramWidget;
    GridWidget * ruleOfThirdsWidget;
public slots:
    // This slot gets hooked up to the CameraThread's viewfinderFrame
    // signal, and updates the visualizations based on the new
    // viewfinder frame.
    void frameAvailable(FCam::Frame);   
};

#endif


