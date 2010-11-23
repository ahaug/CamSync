#include "SettingsTree.h"
//#include "CameraParameters.h"
#include "OverlayWidget.h"

#include <QVBoxLayout>
#include <QButtonGroup>
#include <QPushButton>
#include <QList>

#include <QBitmap>


class CornerWidget : public QWidget {
public:
    CornerWidget(QWidget *parent = 0) : QWidget(parent) {        
    }
protected:
    void paintEvent(QPaintEvent *) {
        QPainter painter(this);
        painter.setPen(OverlayWidget::colorKey());
        /* Draw the following shape:
           #####
           ###
           ##
           #
           #
         */
        painter.drawPoint(1, 1);
        painter.drawPoint(1, 2);
        painter.drawPoint(2, 1);
        painter.drawLine(QPoint(0, 0), QPoint(5, 0));
        painter.drawLine(QPoint(0, 0), QPoint(0, 5));

    }
};


ParameterButton::ParameterButton(QString label, QWidget *parent) :
    QPushButton(label, parent) {
    // There can be only one! (checked at a time)
    for (size_t i = 0; i < instances.size(); i++) {
        QObject::connect(instances[i], SIGNAL(pressed()),
                         this, SLOT(unCheck()));
        QObject::connect(this, SIGNAL(pressed()),
                         instances[i], SLOT(unCheck()));
        
    }
    title = label;
    instances.push_back(this);
    
    
    
    setCheckable(true);
}
void ParameterButton::setSubtitle(QString subtitle) {    
    ModeButton * modeButton = (ModeButton *) group.checkedButton();
    this->setText(QString("%1\n%2 %3").arg(title).arg(modeButton->abbreviatedLabel()).arg(subtitle));
}


std::vector<ParameterButton *> ParameterButton::instances;

ModeButton::ModeButton(QString label, QString abbreviatedLabel, QObject *delegate, ParameterButton *menuParent_, QWidget *parent) : 
    QPushButton(label, parent), menuParent(menuParent_)  {
    setCheckable(true);
    shortLabel = abbreviatedLabel;
    // Whenever anything interesting happens to my parent, recompute my visibility
    QObject::connect(menuParent, SIGNAL(toggled(bool)),
                     this, SLOT(setVisible(bool)));
    QObject::connect(menuParent, SIGNAL(pressed()),
                     this, SLOT(show()));
    QObject::connect(menuParent, SIGNAL(released()),
                     this, SLOT(show()));

    // Inform the delegate when state changes
    QObject::connect(this, SIGNAL(toggled(bool)), 
                     delegate, SLOT(uiChanged()));
                     
    QObject::connect(this, SIGNAL(pressed()), 
                     delegate, SLOT(update()));
    QObject::connect(this, SIGNAL(released()), 
                     delegate, SLOT(update()));
    // Add myself to my parent's menu group
    menuParent->group.addButton(this);

    // place myself in the bottom right of my parent
    int l = menuParent->group.buttons().length();
    int h = minimumSizeHint().height();
    setGeometry(parent->width()-140, parent->height()-h*l, 140, h);
    this->setPalette(menuParent->palette());
    // start hidden
    hide();
        
    // The first child should start checked
    setChecked(true);
}
QString ModeButton::abbreviatedLabel() {
    return shortLabel;
}


void ModeButton::setVisible(bool v) {
    bool old = isVisible();
    if (v && (menuParent->isDown() || menuParent->isChecked())) {
        QPushButton::setVisible(true);
        if (!old) emit changeVisible(true);
    } else {
        QPushButton::setVisible(false);
        if (old) emit changeVisible(false);
    }
}


