#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QDebug>

class MyTcpServer:public QObject
{
    Q_OBJECT
public:
    MyTcpServer(QObject *parent = nullptr);
    ~MyTcpServer();
    void startwrite();
    void stopwrite();
    void Disconnect();
    void set_data(QByteArray data);


private slots:
    void onNewConnection();
    void sendDataToClients();

signals:
    void to_str(QString str,int num);
    void to_lisent(bool ok);
private:
    QByteArray Data;
    QTcpServer *tcpServer;
    QTcpSocket *tcpSocket = nullptr;
    QTimer *sendTimer;
    int num = 0;
    qint64 bytesWritten;    // 已发送字节数
    qint64 blockSize;       // 每个数据块的大小（如64KB）
};

#endif // MYTCPSERVER_H
