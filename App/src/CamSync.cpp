#include <stdio.h>
#include <iostream>

#include "CamSync.h"

CamSync::CamSync(QObject *parent) : QThread(parent)
{
    socket.moveToThread(this);
    host = "128.114.138.51";
    port = 5080;
}

void CamSync::recv(const QString &message)
{
    fprintf(stderr, message.toAscii().data());
    socket.write("picture!\n");
    socket.flush();
}

void CamSync::run()
{
    fprintf(stderr, "CamSync up\n");

    running = true;

    const int Timeout = 5 * 1000;

    socket.connectToHost(host, port);

    if (!socket.waitForConnected(Timeout)) {
        //emit error(socket.error(), socket.errorString());
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
