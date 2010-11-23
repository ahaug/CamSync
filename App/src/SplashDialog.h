#ifndef SPLASH_DIALOG
#define SPLASH_DIALOG

#include <QWidget>
#include <QCheckBox>

/** The dialog box that appears on the first run on FCamera, which
 * helps people get started. */
class SplashDialog : public QWidget {
    Q_OBJECT
public:
    SplashDialog(QWidget * parent = 0);
public slots:
    // Override setVisible to save the user's decision to show or hide
    // this message at program launch.
    void setVisible(bool visible);
protected:
    QCheckBox * checkbox;
};


#endif


