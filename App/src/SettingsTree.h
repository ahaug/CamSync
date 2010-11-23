#ifndef SETTINGS_TREE
#define SETTINGS_TREE

#include <vector>
#include <string>

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QMessageBox>
#include <QtGui>
#include <QMainWindow>
#include <QButtonGroup>
#include <QMutex>

/** This file declares the widgets that make up the settings menu on
 * the right-hand side of the viewfinder screen. See Viewfinder.cpp
 * for how they are used. */

/** The ParameterButtons are always visible on the Viewfinder
 * screen. There's one for exposure, gain, white balance, etc. They
 * each have a group of child ModeButtons, which they show and hide
 * when they are pressed. When a parameter button is pressed, all
 * other parameter buttons are toggled off. */
class ParameterButton : public QPushButton {
    Q_OBJECT
public:
    ParameterButton(QString label, QWidget *parent = 0);

    // The ModeButtons that this parameter button shows and hides.
    QButtonGroup group;    

    // Each ParameterButton has a fixed label given at construction,
    // (e.g. "Exposure"), and a second line which describes the
    // current value of that parameter (e.g. "1/10s").
    void setSubtitle(QString subtitle);
public slots:

    // Toggle the parameter button off (and hide it's ModeButton children)
    void unCheck() {setChecked(false);}
private:

    // The first line of button text, e.g. "Exposure"
    QString title;

    // A vector of all the parameter buttons. This is here so that
    // pressing one parameter button can toggle off every other
    // parameter button.
    static std::vector<ParameterButton *> instances;
};

/** Each ParameterButton owns a group of ModeButtons. For example, the
 * ParameterButton "Exposure" has ModeButtons for "Auto", "Manual",
 * etc. 
 *
 * The ModeButton constructor takes the name of a mode (e.g. "Auto"),
 * and also a suitable abbreviation of that name for display on the
 * second line of the appropriate ParameterButton (e.g. "A").
 *
 * A ModeButton has three logical parents. First, there's the
 * delegate, which is the widget you inform when you get
 * selected. Right now that's always the Viewfinder object. Then,
 * there's the parent in the menu hierachy, which is a
 * ParameterButton. A ModeButton should only be visible if it's
 * menuParent is toggled on. Finally, there's the Qt parent, which
 * is determined by the placement of the ModeButton on the
 * screen. ModeButtons appear over the overlay, so that's their Qt
 * parent. */
class ModeButton : public QPushButton {
    Q_OBJECT
public:
    
    ModeButton(QString label, QString abbreviatedLabel, 
               QObject *delegate, ParameterButton *menuParent, QWidget *parent = 0);    

    // Get the shorthand label, set at construction
    QString abbreviatedLabel();
public slots:
    // The standard QWidget setVisible modified to emit the following changeVisible() signal.
    void setVisible(bool);
signals:
    // Emitted when the visibility of the ModeButton changes. Typically hooked up to any
    // logical child AdjustmentWidgets like sliders or touch areas. 
    void changeVisible(bool);
protected:
    // The abbreviated label displayed in the parameter button parent (e.g. "A" for "Auto")
    QString shortLabel;
    ParameterButton *menuParent;
};



#endif
