#include "tcpthread.h"
#include <QDebug>

TcpThread::TcpThread(QTcpSocket *tcpsocket)
{
    tcpSocket = tcpsocket;
//    tcpServer = new QTcpServer();
//    connect(tcpServer, &QTcpServer::newConnection, this, &TcpThread::onNewConnection);//光电图片数据模拟软件V0.1


//    timer = new QTimer(this);

//    connect(timer,SIGNAL(timeout()),this,SLOT(timerout()));





//    if(!tcpServer->listen(QHostAddress::Any, 16205))
//    {
//        qDebug()<<"未开始监听";
//    }
//    else
//    {
//        qDebug()<<"开始监听";
//    }
}

void TcpThread::onNewConnection()
{
    if(timeout == 0)
    {
        mutex.lock();
        emit info_tcp(true);
        mutex.unlock();
    }

    tcpSocket = tcpServer->nextPendingConnection();
    connect(tcpSocket, &QTcpSocket::disconnected, this, [=]
    {
        //让该线程中的套接字断开连接
        tcpSocket->disconnectFromHost();//断开连接
        Stop_sned();
        mutex.lock();
        emit info_tcp(false);
        mutex.unlock();

        timeout = 3;
        timer->start(1000);

    });
    connect(tcpSocket,  QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &TcpThread::onSocketerror);


}

void TcpThread::timerout()
{
    qDebug()<< timeout;
    if(timeout > 0)
    {
        timeout--;
    }else
    {
        timer->stop();
        Start_sned();
    }
}

void TcpThread::onSocketerror(QAbstractSocket::SocketError socketError)
{
    qDebug() << "Socket error occurred:" << socketError;
//    Stop_sned();
//    mutex.lock();
//    emit info_tcp(false);
//    mutex.unlock();

//    timeout = 3;
//    timer->start(1000);
}


void TcpThread::close_Tcp()
{

    tcpSocket->close();
}

void TcpThread::Stop_sned()
{
    sendok = false;
}

void TcpThread::Start_sned()
{
    sendok = true;
}

void TcpThread::datathread(QByteArray data)
{
    Data = data;
}

void TcpThread::run()
{


    while (sendok) {
        if (!tcpSocket||tcpSocket->state() != QAbstractSocket::ConnectedState)
        {
            qDebug() << "No client connected or socket is not in connected state!";

            return;
        }

        qint64 bytesSent = tcpSocket->write(Data);
        tcpSocket->flush();
        if (bytesSent == -1)
        {
            qDebug()<<"Error sending file!";
            return;
        } else
        {
            qDebug()<<"File sent successfully!";
        }

        tcpSocket->waitForBytesWritten();

        msleep(1000);
    }
    //exec();
}
