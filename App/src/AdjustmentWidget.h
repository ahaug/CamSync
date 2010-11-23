#ifndef ADJUSTMENT_WIDGET
#define ADJUSTMENT_WIDGET

#include <vector>
#include <string>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QtGui>
#include <QMainWindow>
#include <QMutex>
#include <QLabel>

#include "OverlayWidget.h"
#include "CameraParameters.h"
#include "SettingsTree.h"


/** When a mode is selected, its adjustment widget appears over the
 * viewfinder. This class provides a labelled slider, as used by most
 * manual modes. */
class AdjustmentSlider : public QWidget {
    Q_OBJECT
public:
    // As with the ModeButton class defined in SettingsTree.h,
    // AdjustmentSliders have two kinds of parents, logical parents
    // (in this case mode buttons) and true Qt parents (typically the overlay).
    AdjustmentSlider(QString minText, QString maxText, QObject *delegate, ModeButton *menuParent, QWidget *parent = 0);
    // The current position of the slider, with value ranging from 0 to 1000.
    int value();
public slots:
    // Override setVisible check if the menuParent is also visible
    void setVisible(bool);

protected:
    // The text labels at either end of the slider
    QLabel * minLabel, * maxLabel;
    // The mode button that controls the visiblity of this widget    
    ModeButton *menuParent;
    // The actual QSlider that handles the value setting
    QSlider * slider;
};


/** This class provides a means to select a point on the
 * viewfinder. It is used by the spot focus mode. */
class AdjustmentTouchArea : public QWidget {
    Q_OBJECT
public:
    AdjustmentTouchArea(QString prompt, QObject * delegate, ModeButton * menuParent, QWidget *parent = 0);
    // The currently selected point
    QPoint value();
protected:
    // Override mouse events to keep track of mouse movement. For this class,
    // we ignore all but the initial press. If you want active tracking, you can
    // change the move and release event handlers to do something more like mouse
    // press.
    void handleMouseEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event) {handleMouseEvent(event);}
    void mouseReleaseEvent(QMouseEvent * event) {event->accept();}
    void mouseMoveEvent(QMouseEvent * event) {event->accept();}
    void paintEvent(QPaintEvent *);
public slots:
    // Override setVisible to check if the menuParent is also visible
    void setVisible(bool);
signals:
    // Emitted when the selected point has changed. Typically connected to 
    // the Viewfinder's uiChanged() method.
    void valueChanged(QPoint);
private:
    // The currently selected point, values are in the space of this widget,
    // ranging from 0,0 to 639,479 (the pixels covered by the overlay).
    QPoint position;
    // The mode button that controls the visibility of this widget
    ModeButton * menuParent;
};

#endif
