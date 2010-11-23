
#include "SplashDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

#include "UserDefaults.h"

SplashDialog::SplashDialog(QWidget * parent) : QWidget(parent) {

    this->setGeometry(0,0,parent->width(), parent->height());
    QVBoxLayout * vlayout = new QVBoxLayout();
    QHBoxLayout * hlayout = new QHBoxLayout();


    QWidget * dialog = new QWidget();
    dialog->setAutoFillBackground(TRUE);
    QVBoxLayout * dialogVLayout = new QVBoxLayout();
    QHBoxLayout * dialogHLayout = new QHBoxLayout();
    QLabel * title = new QLabel("Welcome to FCamera!");
    title->setAlignment(Qt::AlignHCenter);
    
    QLabel * info = new QLabel("An open source, fully programmable camera.\n"
                               "For information about programming using the\n"
                               "FCam libraries, visit");
    QLabel * url = new QLabel("http://fcam.garage.maemo.org");
    url->setAlignment(Qt::AlignHCenter);                                        
    QLabel * directions = new QLabel("To change camera settings, tap the buttons\n"
                                     "on the right. Captured photographs can be \n"
                                     "reviewed by dragging the viewfinder up and\n"
                                     "additional settings can be accessed by drag-\n"
                                     "ging the viewfinder down.");
    
    dialogVLayout->addWidget(title);
    dialogVLayout->addWidget(info);
    dialogVLayout->addWidget(url);
    dialogVLayout->addWidget(directions);
    
    dialogVLayout->addLayout(dialogHLayout);
    checkbox  = new QCheckBox("Show at launch");
    
    UserDefaults &defaults = UserDefaults::instance();
        
    int hideSplash = 0;
    if (defaults["hideSplash"].valid()) hideSplash = defaults["hideSplash"];

    if (hideSplash){ 
        checkbox->setChecked(FALSE);
        this->hide();
    } else {
        checkbox->setChecked(TRUE);
    }
    
    
    dialogHLayout->addWidget(checkbox);
    //dialogHLayout->addStretch(1);
    QPushButton * dismiss = new QPushButton("Dismiss");
    QObject::connect(dismiss, SIGNAL(clicked()),
                     this, SLOT(hide()));
    dialogHLayout->addWidget(dismiss);
    dialog->setLayout(dialogVLayout);

    hlayout->addStretch(1);
    hlayout->addWidget(dialog);
    hlayout->addStretch(1);

    vlayout->addStretch(1);
    vlayout->addLayout(hlayout);
    vlayout->addStretch(1);
    
    this->setLayout(vlayout);
    
}


void SplashDialog::setVisible(bool visible) {
    QWidget::setVisible(visible);

    // When this box is dismissed, we need to commit settings to disk
    if (!visible) {
        UserDefaults &defaults = UserDefaults::instance();
        defaults["hideSplash"] = checkbox->isChecked() ? 0 : 1;
        defaults.commit();
    }
}
