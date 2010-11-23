#include "ExtendedSettings.h"

#include <QLabel>
#include <QTabBar>
#include <QFrame>
#include <QGridLayout>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>

#include "UserDefaults.h"

#include "CameraThread.h"
extern CameraThread * cameraThread;

ExtendedSettings::ExtendedSettings(QWidget * parent) : QTabWidget(parent) {
    //this->tabBar()->setExpanding(true);
    ignoreSignals = false;
    //QPushButton * quitButton = new QPushButton("X", this);
    //quitButton->setGeometry(800-160, 0, 160, 64);
    //quitButton->setFlat(true);
    //QObject::connect(quitButton, SIGNAL(clicked()),
    //                 cameraThread, SLOT(stop()));
                     
    QFrame * visualization = new QFrame();
    visualization->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout * visLayout = new QVBoxLayout();
    intensityHistogram = new QCheckBox("Show intensity histogram    ");
    visLayout->addWidget(intensityHistogram);    
    ruleOfThirds = new QCheckBox("Show rule of thirds guide    ");
    visLayout->addWidget(ruleOfThirds);
    captureAnimation = new QCheckBox("Show capture animation    ");
    visLayout->addWidget(captureAnimation);
    captureSound = new QCheckBox("Play capture sound    ");
    visLayout->addWidget(captureSound);
    captureBlink = new QCheckBox("Blink LED on capture    ");
    visLayout->addWidget(captureBlink);
    
    visLayout->addStretch(1);
    visualization->setLayout(visLayout);
    

    QFrame * fileManagement = new QFrame();
    fileManagement->setFrameShape(QFrame::StyledPanel);
    QGridLayout * layout = new QGridLayout();
    int row = 0;
    layout->addWidget(new QLabel("RAW Path:"), row, 0);
    rawPath = new QLineEdit();    
    
    //rawPath->setAcceptRichText(false);
    //rawPath->setMaximumSize(QWIDGETSIZE_MAX, 64);
    layout->addWidget(rawPath, row, 1);
    row++;
    
    layout->addWidget(new QLabel("Filename Prefix:"), row, 0);
    filePrefix = new QLineEdit();
    //filePrefix->setMaximumSize(QWIDGETSIZE_MAX, 64);

    layout->addWidget(filePrefix, row, 1);
    row++;
    
    
    QGridLayout * suffixRowLayout = new QGridLayout();
    suffixRowLayout->addWidget(new QLabel("Filename Suffix:"), 0, 0);
    timestamp = new QRadioButton("Timestamp");
    index = new QRadioButton("Index");
    suffixRowLayout->addWidget(timestamp, 0,0);
    suffixRowLayout->addWidget(index, 0,1);
    layout->addWidget(new QLabel("Filename Suffix:"), row, 0);
    layout->addLayout(suffixRowLayout, row, 1);
    row++;
    
    emptyTrashButton = new QPushButton("Erase Trashed Photos");
    restoreTrashButton = new QPushButton("Restore Trashed Photos");
    QHBoxLayout * trashRowLayout = new QHBoxLayout();
    trashRowLayout->addWidget(restoreTrashButton);
    trashRowLayout->addWidget(emptyTrashButton);
    layout->addLayout(trashRowLayout, row, 0, 1, 2);
    row++;
    
    // Add extra spaces at the end of checkbox title due to silly bug autosizing
    autosaveJPGs = new QCheckBox("Autosave JPGs to N900 gallery    ");
    autosaveJPGs->setMaximumSize(QWIDGETSIZE_MAX, 64);
    layout->addWidget(autosaveJPGs, row, 0, 1, 2, Qt::AlignLeft);
    row++;
    
    QPushButton * resetSettings = new QPushButton("Restore Defaults");
    layout->addWidget(resetSettings, row, 1, Qt::AlignRight);

    fileManagement->setLayout(layout);
            
    
    

    QFrame * aboutFCamera = new QFrame();
    aboutFCamera->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout * aboutLayout = new QVBoxLayout();
    QLabel * whatIsFCam = new QLabel(
        "FCamera is a completely open-source N900 camera implementation\n"
        "written using the FCam camera control API. We think you should be\n"
        "able to program your camera to behave any way you want it to.\n\n"
        "Don't like our exposure metering algorithm? Change it.\n"
        "Want a mode for doing time-lapse photography? Write it.\n"
        "Do you desperately want a sepia toned viewfinder? Go nuts.\n\n"
        "With FCamera, you can. To get started, check out our webpage at"        
    );
    whatIsFCam->setAlignment(Qt::AlignJustify);
    QPushButton * url = new QPushButton("http://fcam.garage.maemo.org");
    QObject::connect(url, SIGNAL(clicked()),
                     this, SLOT(launchBrowserForFCamWebsite()));
    url->setFlat(true);
    aboutLayout->addStretch(1);
    aboutLayout->addWidget(whatIsFCam);
    aboutLayout->addWidget(url);
    aboutLayout->addStretch(1);    
    aboutFCamera->setLayout(aboutLayout);

    this->addTab(fileManagement, "File Management");
    this->addTab(visualization, "Visualizations");
    this->addTab(aboutFCamera, "About FCamera");    

    
    this->refreshWidgetsFromDefaults();
    
    QObject::connect(rawPath, SIGNAL(editingFinished()),
                    this, SLOT(settingChanged()));
    QObject::connect(filePrefix, SIGNAL(editingFinished()),
                    this, SLOT(settingChanged()));
    QObject::connect(timestamp, SIGNAL(toggled(bool)),
                    this, SLOT(settingChanged()));
    QObject::connect(index, SIGNAL(toggled(bool)),
                    this, SLOT(settingChanged()));
    QObject::connect(autosaveJPGs, SIGNAL(toggled(bool)),
                    this, SLOT(settingChanged()));
    QObject::connect(resetSettings, SIGNAL(clicked()),
                    this, SLOT(restoreSettingsToDefault()));
    QObject::connect(intensityHistogram, SIGNAL(toggled(bool)),
                    this, SLOT(settingChanged()));
    QObject::connect(ruleOfThirds, SIGNAL(toggled(bool)),
                this, SLOT(settingChanged()));
    QObject::connect(captureAnimation, SIGNAL(toggled(bool)),
                this, SLOT(settingChanged()));
    QObject::connect(captureSound, SIGNAL(toggled(bool)),
                this, SLOT(settingChanged()));
    QObject::connect(captureBlink, SIGNAL(toggled(bool)),
                this, SLOT(settingChanged()));
    
    QObject::connect(emptyTrashButton, SIGNAL(clicked()),
                     this, SLOT(emptyTrash()));
    QObject::connect(restoreTrashButton, SIGNAL(clicked()),
                     this, SLOT(restoreTrash()));
    
    
    
}

void ExtendedSettings::refreshWidgetsFromDefaults(){
    UserDefaults &userDefaults = UserDefaults::instance();
    if (!userDefaults["rawPath"].valid()) {
        userDefaults["rawPath"] = "/home/user/MyDocs/FCamera/";    
    } 
    if (!userDefaults["filenamePrefix"].valid()) {
        userDefaults["filenamePrefix"] = "photo";
    }
    if (!userDefaults["filenameSuffix"].valid()){
        userDefaults["filenameSuffix"] = "timestamp";
    } 
    if (!userDefaults["autosaveJPGs"].valid()) {
        userDefaults["autosaveJPGs"] = false;
    }
    if (!userDefaults["intensityHistogram"].valid()){
        userDefaults["intensityHistogram"] = true;
    }
    if (!userDefaults["ruleOfThirds"].valid()){
        userDefaults["ruleOfThirds"] = false;
    }
    if (!userDefaults["captureAnimation"].valid()){
        userDefaults["captureAnimation"] = true;
    }
    if (!userDefaults["captureSound"].valid()){
        userDefaults["captureSound"] = false;
    }
    if (!userDefaults["captureBlink"].valid()){
        userDefaults["captureBlink"] = false;
    }
    // Block signals temporarily so that all the changes get to propagate
    ignoreSignals = true;
    
    rawPath->setText(userDefaults["rawPath"].asString().c_str());
    filePrefix->setText(userDefaults["filenamePrefix"].asString().c_str());
    if (userDefaults["filenameSuffix"].asString() == "timestamp") {
        timestamp->setChecked(true);
    } else {
        index->setChecked(true);
    }     
    autosaveJPGs->setChecked(userDefaults["autosaveJPGs"].asInt());
    intensityHistogram->setChecked(userDefaults["intensityHistogram"].asInt());
    ruleOfThirds->setChecked(userDefaults["ruleOfThirds"].asInt());
    captureAnimation->setChecked(userDefaults["captureAnimation"].asInt());
    captureSound->setChecked(userDefaults["captureSound"].asInt());
    captureBlink->setChecked(userDefaults["captureBlink"].asInt());
    
    
    // Reactivate signals
    ignoreSignals = false;
    
    this->updateTrashButtons();
    userDefaults.commit();
}


void ExtendedSettings::settingChanged(){
    if (ignoreSignals) return;

    UserDefaults &userDefaults = UserDefaults::instance();
    
    userDefaults["rawPath"] = rawPath->text().toStdString();
    userDefaults["filenamePrefix"] = filePrefix->text().toStdString();    
    
    if (timestamp->isChecked()){
        userDefaults["filenameSuffix"] = "timestamp";
    } else {
        userDefaults["filenameSuffix"] = "index";
    }
    
    userDefaults["autosaveJPGs"] = autosaveJPGs->isChecked();
    userDefaults["intensityHistogram"] = intensityHistogram->isChecked();
    userDefaults["ruleOfThirds"] = ruleOfThirds->isChecked();
    userDefaults["captureAnimation"] = captureAnimation->isChecked();
    userDefaults["captureSound"] = captureSound->isChecked();
    userDefaults["captureBlink"] = captureBlink->isChecked();

    this->updateTrashButtons();
    userDefaults.commit();
}

void ExtendedSettings::restoreSettingsToDefault() {
    UserDefaults &userDefaults = UserDefaults::instance();
    userDefaults.clear();
    userDefaults.commit();
    //printf("after clear, defaults has %d items\n", userDefaults.count());
    
    this->refreshWidgetsFromDefaults();
    //printf("after refresh, defaults has %d items\n", userDefaults.count());

}

void ExtendedSettings::launchBrowserForFCamWebsite() {
    QDesktopServices::openUrl(QUrl("http://fcam.garage.maemo.org"));
}

// Delete all trashed photos permanently.
void ExtendedSettings::emptyTrash() {
    printf("Emptying trash...\n");
    UserDefaults &userDefaults = UserDefaults::instance();
    QDir library(userDefaults["rawPath"].asString().c_str());
    QStringList files = library.entryList();
    foreach (QString file, files) {
        if (file.endsWith(".trash")){
            library.remove(file);        
        }
    }
    this->updateTrashButtons();
}


// Return all trashed photos to the RAW library (possibly renaming
// some to avoid name conflicts.)
void ExtendedSettings::restoreTrash() {
    printf("Restoring trash...\n");
    UserDefaults &userDefaults = UserDefaults::instance();
    QDir library(userDefaults["rawPath"].asString().c_str());
    QStringList files = library.entryList();
    foreach (QString file, files) {
        if (file.endsWith(".dng.trash")){
            QString base = file.left(file.count() - 10); // take off ".dng.trash"
            QString newName = file.left(file.count() - 6); // take off ".trash"
            int dupeCount = 0;
            while (!library.rename(file, newName)) {                
                newName = base + QString().sprintf(".%d.dng", ++dupeCount);
            }
            library.rename(base + ".dng.thumb.trash", newName + ".thumb");
            emit restoredFileAtPath(userDefaults["rawPath"].asString().c_str() + newName);
        }
    }
    this->updateTrashButtons();
}


void ExtendedSettings::updateTrashButtons() {
    restoreTrashButton->setEnabled(false);
    emptyTrashButton->setEnabled(false);
    
    UserDefaults &userDefaults = UserDefaults::instance();
    QDir library(userDefaults["rawPath"].asString().c_str());
    QStringList files = library.entryList();
    foreach (QString file, files) {
        if (file.endsWith(".dng.trash")){
            restoreTrashButton->setEnabled(true);
            emptyTrashButton->setEnabled(true);
        }
    }
}
