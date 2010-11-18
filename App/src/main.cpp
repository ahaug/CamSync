#include <QtGui/QApplication>
#include "mainviewerwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainViewerWindow w;

    w.showMaximized();
    return a.exec();
}
