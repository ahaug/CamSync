#include "mainviewerwindow.h"
#include "ui_mainviewerwindow.h"
#include <QDir>
#include <QLabel>
#include <QPropertyAnimation>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QPainter>
#include <iostream>

MainViewerWindow::~MainViewerWindow()
{
    delete ui;
}

MainViewerWindow::MainViewerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainViewerWindow)
{
    // define available screen area for this main window
    QRect availableGeometry = qApp->desktop()->availableGeometry();
    ui->setupUi(this);
    this->resize(availableGeometry.size());

    // connects for navigation between views
    connect(ui->listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(showImageView(QListWidgetItem*)));

    // start app with file view on screen
    updateFileList();
    ui->stackedWidget->setCurrentIndex(Fileview);

}

void MainViewerWindow::mouseMoveEvent(QMouseEvent* event)
{
    int x, y;
    x = event->x()-ui->imageLabel->x()-(ui->imageLabel->width()/2-image.width()/2);
    y = event->y()-ui->imageLabel->y();

    for (int ix=-3;ix<=3;ix++)
        for (int iy=-3;iy<=3;iy++)
            if (x+ix>0 && x+ix<image.width() && y+iy>0 && y+iy<image.height())
                image.setPixel(x+ix,y+iy,qRgb(255,255,255));

    ui->imageLabel->setPixmap(QPixmap::fromImage(image));
}

void MainViewerWindow::mousePressEvent(QMouseEvent* event)
{

}

void MainViewerWindow::showFileView()
{
    ui->stackedWidget->setCurrentIndex(Fileview);
}

void MainViewerWindow::updateFileList()
{
    QDir dir;
    dir.cd("../src/img");

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        ui->listWidget->addItem(fileInfo.fileName());
    }
}

void MainViewerWindow::showImageView(QListWidgetItem *item)
{
    // set selected image for image label
    image.load("../src/img/"+item->text());
    image = image.scaledToHeight(height());
    ui->imageLabel->setPixmap(QPixmap::fromImage(image));


    // change to image view
    ui->stackedWidget->setCurrentIndex(Imageview);

}

void MainViewerWindow::resizeEvent(QResizeEvent */*event*/)
{
}
\
