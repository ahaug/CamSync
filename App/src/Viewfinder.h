#ifndef FCAMERA_VIEWFINDER_H
#define FCAMERA_VIEWFINDER_H

#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <vector>
#include <QButtonGroup>

#include "SettingsTree.h"
#include "AdjustmentWidget.h"
#include "ImageItem.h"

class CameraThread;
class OverlayWidget;

/** This widget is the main screen of FCamera. It shows an
 * OverlayWidget on the left for the viewfinder frame, and a tree of
 * ParameterButtons, ModeButtons, and AdjustmentWidgets on the right
 * for managing camera state. */
class Viewfinder: public QWidget {
    Q_OBJECT

public:
    Viewfinder(CameraThread *thread, QWidget *parent = 0);

    // Returns the height of the currently displayed adjustmentWidget 
    // (slider, touch area, etc.) so that visualizations can remain
    // visible.
    int adjustmentWidgetHeight();
private:

    // The control thread. Owns the camera parameters state.
    CameraThread *controlThread;

    // The user interface for the viewfinder screen is as follows
    
    // The actual viewfinder on the left
    OverlayWidget *overlay;

    // The quit button in the top right
    QPushButton *quitButton;

    // The always-visible parameter buttons, and the child widgets
    // they create when selected
    struct {
        // The always-visible button
        ParameterButton *button;

        // The sometimes-visible mode buttons
        ModeButton *automatic, *manual, *highlights, *shadows, *hdr;
        
        // The slider for manual mode
        AdjustmentSlider *slider;
    } exposure;

    struct {
        ParameterButton *button;
        ModeButton *automatic, *manual;
        AdjustmentSlider *slider;
    } gain;
    
    struct {
        ParameterButton *button;
        ModeButton *automatic, *manual, *point;
        AdjustmentSlider *slider;        
        AdjustmentTouchArea * touchWidget;
    } focus;

    struct {
        ParameterButton *button;
        ModeButton *automatic, *manual;
        AdjustmentSlider *slider;
    } whiteBalance;

    struct {
        ParameterButton *button;
        ModeButton *single, *continuous, *sharpest;
    } burst;
    
    // A list of all the parameter buttons that makes grabbing
    // the currently selected button easier. Note that this is
    // not a QWidget (it's more like a std::vector of button pointers).
    QButtonGroup * parameterButtonGroup;
    
    // A label containing a pixmap for the just taken photograph. This 
    // gets animated down to the thumbnail view below the viewfinder.
    QLabel *snapshot;
    
protected:
    virtual void paintEvent(QPaintEvent * event);
public slots:

    // Show an animation of the viewfinder frame being pushed down to the thumbnail view
    void animatePhotoTaken(bool wasBurst);

    // the underlying camera parameters changed, need to update the UI
    void parametersChanged();

    // The UI changed, need to update the underlying parameters
    void uiChanged();

private:
    // Update the labels on the parameter buttons
    void updateParameterLabels();
    bool uiChangedSuppressed;
};




#endif
