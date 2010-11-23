#ifndef CAMSYNC_H
#define CAMSYNC_H

#include <QThread>
#include <QMutex>
#include <QtNetwork>

class CamSync : public QThread
{
    Q_OBJECT

public:
    CamSync(QObject *parent = 0);
    void run();
    void recv(const QString &message);

private:
    QTcpSocket socket;
    QString host;
    quint16 port;
    QMutex mutex;
    bool running;
};

#endif // CAMSYNC_H
