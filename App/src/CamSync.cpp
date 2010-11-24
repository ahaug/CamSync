#include <stdio.h>
#include <iostream>
#include <assert.h>

#include "CamSync.h"

CamSync::CamSync(QObject *parent) : QThread(parent)
{
    socket.moveToThread(this);
    host = "128.114.138.51";
    port = 5080;
    QObject::connect(this, SIGNAL(newImage(ImageItem *)),
                     this, SLOT(send(ImageItem *)));
}

void CamSync::recv(const QString &message)
{
    // send a shutter pressed event to get FCamera to take a picture
}

void CamSync::send(ImageItem *imageItem)
{
    // copy the image's data into a buffer and send it to back to the server
}

void CamSync::run()
{
    fprintf(stderr, "CamSync up\n");

    running = true;

    const int Timeout = 5 * 1000;

    socket.connectToHost(host, port);

    if (!socket.waitForConnected(Timeout)) {
        fprintf(stderr, "error connecting to server\n");
        return;
    }

    fprintf(stderr, "connected to server, listening\n");

    while (running) {
        if (socket.canReadLine()) {
            recv(socket.readLine(10000));
        } else {
            socket.waitForReadyRead(100);
        }
    }
}
