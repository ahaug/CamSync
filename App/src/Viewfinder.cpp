#include "Viewfinder.h"
#include "OverlayWidget.h"
#include "CameraThread.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QMessageBox>
#include <QtGui>
#include <QPropertyAnimation>
#include <QMainWindow>

#include <QButtonGroup>

#include "SettingsTree.h"
#include "CameraParameters.h"
#include "AdjustmentWidget.h"
#include "VisualizationWidget.h"
#include "UserDefaults.h"


#include <sstream>

#define TREE_WIDTH 11

Viewfinder::Viewfinder(CameraThread *thread, QWidget *parent):
    QWidget(parent), controlThread(thread), uiChangedSuppressed(true) {

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Make an overlay for displaying viewfinder frames
    overlay = new OverlayWidget(this);
    overlay->setFixedSize(640, 480);

    // Tell camera thread about our overlay
    controlThread->setOverlay(overlay);

    layout->addWidget(overlay);
    // Make space for the parameter button tree drawing
    layout->addSpacing(TREE_WIDTH);        
    
    // Make some buttons down the right              
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(0);
    layout->addLayout(buttonLayout);

    buttonLayout->addWidget(quitButton = new QPushButton("Quit", this));
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(exposure.button     = new ParameterButton("Exposure", this));
    buttonLayout->addWidget(gain.button         = new ParameterButton("Gain", this));
    buttonLayout->addWidget(focus.button        = new ParameterButton("Focus", this));
    buttonLayout->addWidget(whiteBalance.button = new ParameterButton("WB", this));
    buttonLayout->addWidget(burst.button        = new ParameterButton("Burst", this));
    
    parameterButtonGroup = new QButtonGroup();
    parameterButtonGroup->setExclusive(FALSE);
    parameterButtonGroup->addButton(exposure.button);
    parameterButtonGroup->addButton(gain.button);
    parameterButtonGroup->addButton(focus.button);
    parameterButtonGroup->addButton(whiteBalance.button);
    parameterButtonGroup->addButton(burst.button);
    QObject::connect(parameterButtonGroup, SIGNAL(buttonPressed(QAbstractButton *)),
                     this, SLOT(update()));
    QObject::connect(parameterButtonGroup, SIGNAL(buttonReleased(QAbstractButton *)),
                     this, SLOT(update()));
    
    // Make the quit button look different to the others
    quitButton->setFlat(TRUE);
    

                     
    // Now make the mode buttons for each parameter as children of the
    // overlay. They go in the bottom right corner of it stacked up.
    exposure.manual     = new ModeButton("Manual", "M:", this, exposure.button, overlay);
    exposure.highlights = new ModeButton("Highlights", "Hi:", this, exposure.button, overlay);
    exposure.shadows    = new ModeButton("Shadows", "Sh:", this, exposure.button, overlay);
    exposure.hdr        = new ModeButton("HDR", "HDR:", this, exposure.button, overlay);
    exposure.automatic  = new ModeButton("Auto", "A:", this, exposure.button, overlay);
    exposure.slider     = new AdjustmentSlider("1/8000s", "1s", this, exposure.manual, overlay);
    
    gain.manual    = new ModeButton("Manual", "M:", this, gain.button, overlay);
    gain.automatic = new ModeButton("Auto", "A:", this, gain.button, overlay);
    gain.slider    = new AdjustmentSlider("ISO 100", "ISO 3200", this, gain.manual, overlay);

    focus.manual    = new ModeButton("Manual", "M:", this, focus.button, overlay);
    focus.point     = new ModeButton("Point", "P:",this, focus.button, overlay);
    focus.automatic = new ModeButton("Auto", "A:",this, focus.button, overlay);
    focus.slider    = new AdjustmentSlider("5cm", ">5m", this, focus.manual, overlay);
    focus.touchWidget = new AdjustmentTouchArea("Touch to select AF point", this, focus.point, overlay);
    
    whiteBalance.manual    = new ModeButton("Manual", "M:", this, whiteBalance.button, overlay);
    whiteBalance.automatic = new ModeButton("Auto", "A:", this, whiteBalance.button, overlay);
    whiteBalance.slider    = new AdjustmentSlider("3000K", "8000K", this, whiteBalance.manual, overlay);    
    
    burst.sharpest   = new ModeButton("Best of 8", "Best of 8", this, burst.button, overlay);
    burst.continuous = new ModeButton("Burst of 4", "Burst of 4", this, burst.button, overlay);
    burst.single     = new ModeButton("Single", "Single", this, burst.button, overlay);

    
    this->setLayout(layout);

    // Hook up the quit button to the camera control thread
    QObject::connect(quitButton, SIGNAL(clicked()),
                     controlThread, SLOT(stop()));

    QObject::connect(& controlThread->parameters, SIGNAL(changed()),
                     this, SLOT(parametersChanged()));
    uiChangedSuppressed = false;
    this->uiChanged();
    
    // Add the histogram widget to the overlay
    VisualizationWidget * visualizationWidget = new VisualizationWidget(this, overlay);
    visualizationWidget->setFixedSize(640,480);
    QObject::connect(controlThread, SIGNAL(viewfinderFrame(FCam::Frame)),
                     visualizationWidget, SLOT(frameAvailable(FCam::Frame)));
                     
                     
                     
    QLabel * savingNotice = new QLabel("Saving...", this);
    savingNotice->setAutoFillBackground(TRUE);
    savingNotice->setAlignment(Qt::AlignCenter);
    savingNotice->setGeometry(this->width()/3,this->height()/2-16,this->width()/3, 32);
    
    savingNotice->hide();
    QObject::connect(quitButton, SIGNAL(clicked()),
                     savingNotice, SLOT(show()));
                     
    snapshot = new QLabel(this);
    snapshot->setFrameShape(QFrame::Box);
    snapshot->setGeometry(640,480,0,0);
    snapshot->setAutoFillBackground(TRUE);
    QPalette palette = snapshot->palette();
    palette.setColor(QPalette::Window, QColor("grey"));
    snapshot->setPalette(palette);
}
    
void Viewfinder::paintEvent(QPaintEvent * event) {
    QWidget::paintEvent(event);
    //printf("viewfinder paint event\n");
    // Horrible hack to get the tree drawn
    QPainter painter(this);
    QPalette palette;

    QPen activePen(painter.pen());
    activePen.setWidth(3);
    activePen.setColor(palette.color(QPalette::Highlight));    
    QPen passivePen(painter.pen());
    passivePen.setWidth(3);
    passivePen.setColor(QColor(64,64,64));
    
    
    
    int centerX  = overlay->width()+TREE_WIDTH/2;
    ParameterButton * selectedParameterButton = NULL;
    foreach (QAbstractButton * button, parameterButtonGroup->buttons()) {
        if (button->isDown() || button->isChecked()) {
            selectedParameterButton = (ParameterButton *) button;
            break;
        }    
    }    
    if (selectedParameterButton){
        int parameterY = selectedParameterButton->geometry().center().y();
        int minSelectedY = parameterY;
        int maxSelectedY = parameterY;
        int minY = parameterY;
        int maxY = parameterY;
        // hash mark for parameter button
        painter.setPen(activePen);
        painter.drawLine(QPoint(centerX, parameterY), QPoint(overlay->width()+TREE_WIDTH, parameterY));
        
        // hash marks for mode buttons
        foreach (QAbstractButton * modeButton, selectedParameterButton->group.buttons()){
            int buttonY = modeButton->geometry().center().y();
            if (modeButton->isChecked() || modeButton->isDown()) {
                painter.setPen(activePen);
                minSelectedY = qMin(buttonY, minSelectedY);
                maxSelectedY = qMax(buttonY, maxSelectedY);      
            } else {
                painter.setPen(passivePen);
            }
            painter.drawLine(QPoint(overlay->width(), buttonY), QPoint(centerX, buttonY));
            minY = qMin(buttonY, minY);
            maxY = qMax(buttonY, maxY);
        }   
        painter.setPen(passivePen);
        painter.drawLine(QPoint(centerX, minY), QPoint(centerX,maxY));  
        painter.setPen(activePen);
        painter.drawLine(QPoint(centerX, minSelectedY), QPoint(centerX, maxSelectedY));
    }
}

void Viewfinder::uiChanged() {
    if (uiChangedSuppressed) return;
    CameraParameters *model = &controlThread->parameters; 

    model->mutex.lock();

    // Exposure
    if (exposure.manual->isChecked()) {
        model->exposure.mode = CameraParameters::Exposure::MANUAL;
        // Compute the exposure time from the slider position using a power function
        float left = 1/8000.0f;
        float middle = 1/30.0f;
        float right = 1.0f;
        float beta = log2f((right-left)/(middle-left));
        float alpha = (middle + right - 2*left)/(powf(500, beta)+powf(1000, beta));
        float gamma = left;
        model->exposure.value = powf((float)exposure.slider->value(), beta)*alpha + gamma;
    } else if (exposure.automatic->isChecked()) {
        model->exposure.mode = CameraParameters::Exposure::AUTO;
    } else if (exposure.highlights->isChecked()) {
        model->exposure.mode = CameraParameters::Exposure::HIGHLIGHTS;
    } else if (exposure.shadows->isChecked()) {
        model->exposure.mode = CameraParameters::Exposure::SHADOWS;
    } else if (exposure.hdr->isChecked()) {
        model->exposure.mode = CameraParameters::Exposure::AUTO_HDR;
    }

    // Gain
    if (gain.manual->isChecked()) {
        model->gain.mode = CameraParameters::Gain::MANUAL;
        // use a log scale for this one
        model->gain.value = powf(2, gain.slider->value()/200.0);
    } else {
        model->gain.mode = CameraParameters::Gain::AUTO;
    }

    // Focus
    if (focus.manual->isChecked()) {
        model->focus.mode = CameraParameters::Focus::MANUAL;
        // linear in diopters with infinity (zero diopters) on the right
        model->focus.value = (1000-focus.slider->value())/50.0;        
    } else if (focus.automatic->isChecked()) {
        model->focus.mode = CameraParameters::Focus::AUTO;
    } else { // spot
        model->focus.mode = CameraParameters::Focus::SPOT;
        model->focus.spot = focus.touchWidget->value();
    }
    
    // White balance
    if (whiteBalance.manual->isChecked()) {
        model->whiteBalance.mode = CameraParameters::WhiteBalance::MANUAL;
        // linear in Kelvin from 3000K to 8000K
        model->whiteBalance.value = 3000 + whiteBalance.slider->value()*5;
    } else {
        model->whiteBalance.mode = CameraParameters::WhiteBalance::AUTO;
    }

    // Burst
    if (burst.single->isChecked()) {
        model->burst.mode = CameraParameters::Burst::SINGLE;
    } else if (burst.continuous->isChecked()) {
        model->burst.mode = CameraParameters::Burst::CONTINUOUS;
    } else { // sharpest
        model->burst.mode = CameraParameters::Burst::SHARPEST;
    }

    model->mutex.unlock();
    this->update();
    updateParameterLabels();
}


void Viewfinder::parametersChanged() {
    //printf("parametersChanged\n");
    updateParameterLabels();
}

void Viewfinder::animatePhotoTaken(bool wasBurst) {
    if (! UserDefaults::instance()["captureAnimation"].asInt()) {
        return;
    }
    printf("animation being built\n");
    
    QPropertyAnimation * animation = new QPropertyAnimation(snapshot, "geometry", this);
    animation->setDuration(400); // in milliseconds
    animation->setStartValue(QRect(0,0,640,480));
    animation->setEndValue(QRect(160,480,320,240));
   // animation->setEasingCurve(QEasingCurve::InOutElastic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    printf("animation started!\n");
}


void Viewfinder::updateParameterLabels() {
    CameraParameters *model = &controlThread->parameters; 
    model->mutex.lock();
    exposure.button->setSubtitle(model->exposure.toString(model->exposure.value));
    gain.button->setSubtitle(model->gain.toString(model->gain.value));
    focus.button->setSubtitle(model->focus.toString(model->focus.value));
    whiteBalance.button->setSubtitle(model->whiteBalance.toString(model->whiteBalance.value));
    burst.button->setSubtitle("");
    model->mutex.unlock();   
}

int Viewfinder::adjustmentWidgetHeight() {
    if (exposure.slider->isVisible() || gain.slider->isVisible() || focus.slider->isVisible() ||
        whiteBalance.slider->isVisible()) {
        return exposure.slider->height();    
    }
    if (focus.touchWidget->isVisible()) {
        return 30; // The height of the QLabel at the bottom of the touch area.
    }
    return 0;
}

