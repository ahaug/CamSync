#include "VisualizationWidget.h"


#include <QColor>
#include <QPainter>

#include <FCam/N900.h>
#include <math.h>
#include "UserDefaults.h"

GridWidget::GridWidget(unsigned int numRows, unsigned int numCols, QWidget * parent) : QWidget(parent) {
    rows = numRows;
    cols = numCols;
}


void GridWidget::paintGrid(QPen pen) {
    QPainter painter(this);
    painter.setPen(pen);
    for (unsigned int r = 1; r < rows; ++r) {
        int cellHeight = this->height() / rows;
        int y = r * cellHeight;
        painter.drawLine(0, y, this->width(), y);
    }
    
    for (unsigned int c = 1; c < cols; ++c) {
        int cellWidth = this->width() / cols;
        int x = c * cellWidth;
        painter.drawLine(x,0, x, this->height());
    }
}

void GridWidget::paintEvent(QPaintEvent * event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    
    QPen background(QColor("black"));
    background.setWidth(3);
    
    this->paintGrid(background);
    this->paintGrid(QPen(QColor("white")));    

}

HistogramWidget::HistogramWidget(QWidget * parent) : QWidget(parent) {

}
void HistogramWidget::setHistogram(FCam::Histogram hist) {
    histogram = hist;
    this->update();
}
void HistogramWidget::paintEvent(QPaintEvent * event) {
    QWidget::paintEvent(event);

    QPainter painter(this); 
    if (histogram.valid()) {
        int margin = 2;
        
        int totalHistogramHeight = this->height()-2*margin;
        int bucketPixelWidth = (this->width()-2*margin)/histogram.buckets();
        int endBucketBonus = (this->width()-2*margin - bucketPixelWidth*histogram.buckets())/2;            
        int largestBucketValue = 0;
        unsigned int i;
        int xPosition;
        for (i = 0; i < histogram.buckets(); ++i) {
            largestBucketValue = qMax((unsigned) largestBucketValue, histogram(i));
            //printf("largestBucketValue = %d\n", largestBucketValue);
        }
        float scale = (float) totalHistogramHeight /  (float) largestBucketValue;

        // Draw the black background
        i = 0;
        xPosition = margin;        
        QPen pen = painter.pen();
        pen.setColor(QColor("black"));
        painter.setPen(pen);
        
        painter.fillRect(xPosition-margin,floor(totalHistogramHeight - histogram(i)*scale),
                         bucketPixelWidth+endBucketBonus+2*margin, ceil(histogram(i)*scale)+2*margin, 
                         QColor("black"));
        xPosition += bucketPixelWidth+endBucketBonus;

        for (i = 1; i < histogram.buckets() -1; ++i){            
            painter.fillRect(xPosition-margin,floor(totalHistogramHeight - histogram(i)*scale),
                             bucketPixelWidth+2*margin, ceil(histogram(i)*scale)+2*margin, 
                             QColor("black"));
            xPosition += bucketPixelWidth;
        }

        painter.fillRect(xPosition-margin,floor(totalHistogramHeight - histogram(i)*scale),
                         bucketPixelWidth+endBucketBonus+2*margin, ceil(histogram(i)*scale)+2*margin, 
                         QColor("black"));
        

        // Draw the white histogram
        i = 0;
        xPosition = margin;
                        
        painter.fillRect(xPosition,floor(totalHistogramHeight - histogram(i)*scale)+margin,
                         bucketPixelWidth+endBucketBonus, ceil(histogram(i)*scale), 
                         QColor("white"));
        xPosition += bucketPixelWidth+endBucketBonus;

        for (i = 1; i < histogram.buckets() -1; ++i){            
            painter.fillRect(xPosition, floor(totalHistogramHeight -  histogram(i)*scale)+margin,
                             bucketPixelWidth, ceil(histogram(i)*scale), 
                             QColor("white"));
            xPosition += bucketPixelWidth;
        }

        painter.fillRect(xPosition,floor(totalHistogramHeight -  histogram(i)*scale)+margin,
                         bucketPixelWidth+endBucketBonus, ceil(histogram(i)*scale), 
                         QColor("white"));
        xPosition += bucketPixelWidth*3;
    }
}



VisualizationWidget::VisualizationWidget(Viewfinder * vf, QWidget * parent) : QWidget(parent) {
    viewfinder = vf;

    // Put me immediately on top of my parent
    this->lower();

    // Why is this line here? - AA
    //parent->lower();    

    // Construct the various visualization widgets as hidden
    ruleOfThirdsWidget = new GridWidget(3,3, this);
    ruleOfThirdsWidget->hide();    
    intensityHistogramWidget = new HistogramWidget(this);    
    intensityHistogramWidget->hide();


}

void VisualizationWidget::frameAvailable(FCam::Frame frame) {
    
    // Figure out which widgets should be visible based on the user defaults
    UserDefaults &userDefaults = UserDefaults::instance();
    if (userDefaults["intensityHistogram"].valid() &&
        (int)userDefaults["intensityHistogram"]) {
        intensityHistogramWidget->show();
    } else {
        intensityHistogramWidget->hide();
    }
    
    if (userDefaults["ruleOfThirds"].valid() &&
        (int)userDefaults["ruleOfThirds"]) {
        ruleOfThirdsWidget->show();
    } else {
        ruleOfThirdsWidget->hide();
    }

    // Figure out where each widget should go, based on which ones are visible
    int widgetFloor = 480 - viewfinder->adjustmentWidgetHeight();

    if (intensityHistogramWidget->isVisible()) {
        intensityHistogramWidget->setGeometry(0, widgetFloor - 64,
                                              (640-140), 64);        
    }
    if (ruleOfThirdsWidget->isVisible()) {
        ruleOfThirdsWidget->setGeometry(0,0, this->width(), this->height());
    }
    // Use the frame to compute any necessary data for each visualization widget
    if (frame.histogram().valid() &&
        intensityHistogramWidget->isVisible()) {
        // Generate a gamma curved histogram
        int buckets = frame.histogram().buckets();
        float gamma = 0.6;
        int supersamplingFactor = 5;
        FCam::Histogram supersampled((buckets-2)*supersamplingFactor + 2 ,1, frame.histogram().region());
        // Copy the zero intensity bin directly
        supersampled(0, 0) = frame.histogram()(0)*supersamplingFactor;
        for (unsigned int bin = 0; bin < frame.histogram().buckets()-2; ++bin) {
            for (int subbin = 0; subbin < supersamplingFactor; ++subbin){
                // This scales the total mass, but it's just for display so it doesn't matter 
                supersampled(supersamplingFactor*bin + subbin + 1, 0) = frame.histogram()(bin+1);
            }            
        }
        // Copy the full intensity bin as well
        supersampled(supersampled.buckets()-1, 0) = frame.histogram()(buckets-1)*supersamplingFactor;
        
        //histogram = supersampled; this->update(); return;
        FCam::Histogram histogram = FCam::Histogram(buckets, 1, frame.histogram().region());
        float inverseBuckets = 1.0f/(supersampled.buckets() - 1);
        for (unsigned int i = 0; i < supersampled.buckets(); ++i){
            float sourceIntensity = i*inverseBuckets;
            float destinationIntensity = powf(sourceIntensity, gamma);
            float idealDestinationBin = destinationIntensity*(buckets-1);
            float lowerBinWeight = idealDestinationBin - floor(idealDestinationBin);
            float upperBinWeight = 1.0f - lowerBinWeight;
            int lowerBin = floor(idealDestinationBin);
            int upperBin = ceil(idealDestinationBin);
            histogram(lowerBin, 0) += supersampled(i) * lowerBinWeight;
            histogram(upperBin, 0) += supersampled(i) * upperBinWeight;            
        }
        // Hack to make bin #1 and #2 look nicer since resampling seems to leave them empty.
        histogram(1,0) = (histogram(0)*0.75f + histogram(3)*0.25f);
        histogram(2,0) = (histogram(0)*0.25f + histogram(3)*0.75f);
        
        intensityHistogramWidget->setHistogram(histogram);
    }

    
}






