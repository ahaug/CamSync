#ifndef EXTENDED_SETTINGS
#define EXTENDED_SETTINGS

#include <QTabWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QPushButton>

/** The tabbed dialog above the viewfinder that provides access to
 * miscellaneous settings and the about page. */
class ExtendedSettings : public QTabWidget {
    Q_OBJECT
public:
    ExtendedSettings(QWidget * parent = 0);
    
protected:
    // Modify all the setting widgets' state to match the "factory" defaults
    void refreshWidgetsFromDefaults();

    // File management panel widgets
    QLineEdit * rawPath, * filePrefix;
    QRadioButton * timestamp, * index;
    QCheckBox * autosaveJPGs;
    QPushButton * emptyTrashButton, * restoreTrashButton;
    
    // Visualization panel widgets
    QCheckBox * intensityHistogram, *ruleOfThirds, *captureAnimation, *captureSound, *captureBlink;

    // While this is true, the slot methods will return without taking any
    // action. This prevents setting change signals from interrupting updates
    // to the UserSettings object.
    bool ignoreSignals;
public slots:
    // Updates UserDefaults to match the state of the settings widgets.
    // Typically connected to all the settings widget change signals.
    void settingChanged();
    // Deletes the existing UserDefaults and replaces it with factory 
    // settings. Hooked up to the reset settings button in the file management tab.
    void restoreSettingsToDefault();
    // Launches a web browser pointed to the FCam website. Hooked up to
    // the URL button in the about tab.
    void launchBrowserForFCamWebsite();
    // Delete all trashed photos permanently.
    void emptyTrash();
    // Return all trashed photos to the RAW library (possibly renaming
    // some to avoid name conflicts.)
    void restoreTrash();
    // This checks whether there are any .trash files in the library and
    // sets the trash managing buttons enabled state appropriately.
    void updateTrashButtons();
    
signals:
    // This signal is emitted for each file that has been restored from the trash.
    // This will get connected with the thumbnail view's addImageAtPath slot.
    void restoredFileAtPath(QString);
};

#endif
