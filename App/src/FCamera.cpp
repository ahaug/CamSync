#include <QApplication>
#include <QObject>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QPalette>
#include <QMessageBox>
#include <FCam/Event.h>

#include "OverlayWidget.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "Viewfinder.h"
#include "ThumbnailView.h"
#include "CameraThread.h"
#include "ScrollArea.h"

#include "AdjustmentWidget.h"
#include "SplashDialog.h"
#include "ExtendedSettings.h"

#include "PanicHandler.h"

CameraThread *cameraThread;

// Handle Ctrl-C with a clean quit
void sigint(int) {
    cameraThread->stop();
}
 
int main(int argc, char **argv) {
    QApplication app(argc, argv);

    // We're going to be passing around Frames using Qt Signals, so we
    // need to first register the type with Qt.
    qRegisterMetaType<FCam::Frame>("FCam::Frame");
    qRegisterMetaType<FCam::Event>("FCam::Event");
    
    // Make a thread that controls the camera and maintains its state
    cameraThread = new CameraThread();
   
    QMainWindow mainWindow;
    VScrollArea *scrollArea = new VScrollArea(&mainWindow);
    scrollArea->setGeometry(0, 0, 800, 480);
    Viewfinder *viewfinder = new Viewfinder(cameraThread);
    ThumbnailView *review = new ThumbnailView();
    ExtendedSettings * extendedSettings = new ExtendedSettings();
    scrollArea->addWidget(extendedSettings);
    scrollArea->addWidget(viewfinder);
    scrollArea->addWidget(review);
    scrollArea->jumpTo(1);
    // Hook up camera thread image signal to the review widget
    QObject::connect(cameraThread, SIGNAL(newImage(ImageItem *)),
                     review, SLOT(newImage(ImageItem *)) );
    // Hook up camera thread image signal to the viewfinder photo taken animation
    QObject::connect(cameraThread, SIGNAL(captureComplete(bool)),
                     viewfinder, SLOT(animatePhotoTaken(bool)));
                     
    // When the shutter of focus button is pressed, the view should
    // slide back to the viewfinder. The cameraThread doesn't know
    // about the scrollArea, so we send the signal via a signal mapper
    // which provides it.
    QSignalMapper mapper;
    mapper.setMapping(cameraThread, viewfinder);
    QObject::connect(cameraThread, SIGNAL(focusPressed()),
                     &mapper, SLOT(map()));
    QObject::connect(cameraThread, SIGNAL(shutterPressed()),
                     &mapper, SLOT(map()));
    QObject::connect(&mapper, SIGNAL(mapped(QWidget *)),
                     scrollArea, SLOT(slideTo(QWidget *)));

    // Connect the threads together in a daisy chain to get them all
    // to stop and then quit the app
    QObject::connect(cameraThread, SIGNAL(finished()), 
                     &IOThread::writer(), SLOT(stop()));
    QObject::connect(&IOThread::writer(), SIGNAL(finished()),
                     &IOThread::reader(), SLOT(stop()));
    QObject::connect(&IOThread::reader(), SIGNAL(finished()),
                     &app, SLOT(quit()));
    
    PanicHandler * panicHandler = new PanicHandler();
    
    QObject::connect(cameraThread, SIGNAL(panic(FCam::Event)),
                     panicHandler, SLOT(handleEvent(FCam::Event)));
    QObject::connect(panicHandler, SIGNAL(eventHandled(FCam::Event)),
                     cameraThread, SLOT(failGracefully()));

    QObject::connect(extendedSettings, SIGNAL(restoredFileAtPath(QString)),
                     review, SLOT(addImageAtPath(QString)));
    QObject::connect(review, SIGNAL(imageTrashed()),
                     extendedSettings, SLOT(updateTrashButtons()));

    QLabel * lensCoverWarning = new QLabel("The lens cover appears shut.\n"
                                            "You might want to open it before taking pictures.", viewfinder);
    lensCoverWarning->setGeometry(40,240-32,540,64);
    //lensCoverWarning->setGeometry(0,0,640,480);
    lensCoverWarning->setAutoFillBackground(true);
    lensCoverWarning->setAlignment(Qt::AlignCenter);
    lensCoverWarning->setVisible(CameraThread::lensCovered());
    
    QObject::connect(cameraThread, SIGNAL(lensCoverClosed()),
                     lensCoverWarning, SLOT(show()));
    QObject::connect(cameraThread, SIGNAL(lensCoverOpened()),
                     lensCoverWarning, SLOT(hide()));
                     
    
    
    new SplashDialog(&mainWindow);
    
    //review->launchShareDialog();
    // Launch the camera control thread
    cameraThread->start();

    // Make Ctrl-C call app->exit(SIGINT)
    signal(SIGINT, sigint);

    mainWindow.showFullScreen();
    int rval = app.exec();

    printf("About to delete camera thread\n");

    delete cameraThread;
    printf("Camera thread deleted\n");
    return rval;
}


