#ifndef TCPTHREAD_H
#define TCPTHREAD_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QMutex>

#include <QTimer>

class TcpThread:public QThread
{
    Q_OBJECT
public:
    TcpThread( QTcpSocket *tcpsocket);
    void datathread(QByteArray data);
    void close_Tcp();
    void Stop_sned();
    void Start_sned();
private slots:
    void onNewConnection();
    void timerout();
    void onSocketerror(QAbstractSocket::SocketError socketError);






protected:
    void run() override;

signals:
    void info_tcp(bool info);

private:
    QMutex mutex;
    QByteArray Data;
    QTcpServer *tcpServer;
    QTcpSocket *tcpSocket = nullptr;
    int timeout = 0;

    QTimer *timer;

    bool sendok = true;
};

#endif // TCPTHREAD_H
