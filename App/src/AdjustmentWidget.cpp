#include "AdjustmentWidget.h"
//#include "CameraParameters.h"
#include "OverlayWidget.h"

#include <QVBoxLayout>
#include <QButtonGroup>
#include <QPushButton>
#include <QList>

#include <QBitmap>
#include <QPalette>


QPoint AdjustmentTouchArea::value(){
    return position;
}

void AdjustmentTouchArea::handleMouseEvent(QMouseEvent * event) {
    position = event->pos();
    emit valueChanged(event->pos());
    this->update();
    event->accept();
}

void AdjustmentTouchArea::paintEvent(QPaintEvent *) {
    //printf("paint is being called, but I am visible? %d\n", this->isVisible());
    QPainter painter(this);
    int r = 10;
    QPen pen;
    pen.setWidth(2);
    pen.setColor(QColor("white"));
    painter.setPen(pen);
    painter.drawEllipse(position, r,r);
    pen.setColor(QColor("black"));
    painter.setPen(pen);
    painter.drawEllipse(position, r+2,r+2);                
}
    
AdjustmentTouchArea::AdjustmentTouchArea(QString prompt, QObject * delegate, ModeButton * menuParent_, QWidget *parent) :       
                                    QWidget(parent) {
    
    menuParent = menuParent_;
    QObject::connect(menuParent, SIGNAL(toggled(bool)),
                     this, SLOT(setVisible(bool)));
    QObject::connect(menuParent, SIGNAL(changeVisible(bool)),
                     this, SLOT(setVisible(bool)));
    QObject::connect(menuParent, SIGNAL(pressed()),
                     this, SLOT(show()));
    QObject::connect(menuParent, SIGNAL(released()),
                     this, SLOT(show()));                     
    QObject::connect(this, SIGNAL(valueChanged(QPoint)),
                     delegate, SLOT(uiChanged()));
                         
    this->setGeometry(0,0, parent->width(), parent->height());
    
    
    QWidget * labelWrapper = new QWidget(this);
    this->setPalette(QApplication::palette());
    labelWrapper->setAutoFillBackground(TRUE);

    QLabel * promptLabel = new QLabel(prompt);
    int h = promptLabel->minimumSizeHint().height();

    labelWrapper->setGeometry(0, parent->height()-h, menuParent->x(), h);
    QHBoxLayout * hlayout = new QHBoxLayout();
    
    hlayout->addStretch(1);
    hlayout->addWidget(promptLabel);
    hlayout->addStretch(1);
    labelWrapper->setLayout(hlayout);  
    
    position.setX(parent->width()/2);
    position.setY(parent->height()/2);
    this->lower();
    parent->lower();
    this->hide();
}

void AdjustmentTouchArea::setVisible(bool v) {
    if (v && (menuParent->isDown() || menuParent->isChecked()))
        QWidget::setVisible(true);
    else
        QWidget::setVisible(false);
}


AdjustmentSlider::AdjustmentSlider(QString minText, QString maxText, QObject *delegate, 
                                 ModeButton *menuParent_, QWidget *parent) : 
                                 QWidget(parent), menuParent(menuParent_) {

    // Whenever anything interesting happens to my parent, recompute my visibility
    QObject::connect(menuParent, SIGNAL(toggled(bool)),
                     this, SLOT(setVisible(bool)));
    QObject::connect(menuParent, SIGNAL(changeVisible(bool)),
                     this, SLOT(setVisible(bool)));
    QObject::connect(menuParent, SIGNAL(pressed()),
                     this, SLOT(show()));
    QObject::connect(menuParent, SIGNAL(released()),
                     this, SLOT(show()));
    this->setPalette(QApplication::palette());
    
    
    slider = new QSlider(this);
    
    // Inform the delegate when state changes
    QObject::connect(slider, SIGNAL(valueChanged(int)), 
                     delegate, SLOT(uiChanged()));
    slider->setMinimum(0);
    slider->setMaximum(1000);
    slider->setValue(500);
    
    minLabel = new QLabel(minText, this);
    maxLabel = new QLabel(maxText, this);

    slider->setOrientation(Qt::Horizontal);
    //slider->setPalette(menuParent->palette());
    
    
    this->setAutoFillBackground(TRUE); 
    
    // put myself across the bottom of my QT parent
    int h = slider->minimumSizeHint().height() + minLabel->minimumSizeHint().height();
    this->setGeometry(0, parent->height()-h, menuParent->x(), h);
    
    
    QWidget * labelWrapper = new QWidget();
    QHBoxLayout * labelLayout = new QHBoxLayout();
    labelLayout->setContentsMargins(8,0,8,0);
    labelLayout->setSpacing(0);
    
    labelLayout->addWidget(minLabel);
    labelLayout->addStretch(1);
    
    labelWrapper->setAutoFillBackground(true);
    labelLayout->addWidget(maxLabel);
    labelWrapper->setLayout(labelLayout);
    
    QVBoxLayout * layout = new QVBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->addWidget(labelWrapper);
    layout->addWidget(slider);
    this->setLayout(layout);
    // start hidden
    hide();
}

void AdjustmentSlider::setVisible(bool v) {
    if (v && (menuParent->isDown() || menuParent->isChecked()))
        QWidget::setVisible(true);
    else
        QWidget::setVisible(false);
}

int AdjustmentSlider::value() {
    return slider->value();
}
