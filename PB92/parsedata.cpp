#include <QDebug>
#include <QApplication>

#include "parsedata.h"



ParseData::ParseData(QObject *parent) :
    QObject(parent)
{

  tcpServer = new QTcpServer(this);
  connect(tcpServer, SIGNAL(newConnection()), this, SLOT(NewConnectionSlot()));
  tcpServer->listen(QHostAddress::Any, 16205);
  bconnet =false;
}
ParseData::~ParseData()
{


}

void ParseData::NewConnectionSlot()
{
    currentClient = tcpServer->nextPendingConnection();
    //currentClient->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,1024*1024*1024);
    buff = new char [60*1024*1024] ();
    memset(buff,0xAA,60*1024*1024);
    //connect(currentClient, SIGNAL(readyRead()), this, SLOT(Send()));
  //  connect(currentClient, SIGNAL(disconnected()), this, SLOT(disconnectedSlot()));
     bconnet =true;
   // ui->pushButton_18->setEnabled(true);
   // ui->pushButton_21->setEnabled(true);

}
 void ParseData::onDataChanged()
 { 
     if(bconnet)
     {
         //if(currentClient->waitForBytesWritten())
         {
            QCoreApplication::processEvents();
             int size = currentClient->write(buff,60*1024*1024);
             //currentClient->flush();
              currentClient->waitForBytesWritten();
             //currentClient->waitForReadyRead();

             qDebug() << size;
         }

     }

 }
/*
void ParseData::run()
{

   QByteArray Narry;
  // Narry.setNum(1;
   Narry.append(1);
   Narry.append(2);
   Narry.append(2);
  // memcpy(Narry,MainWindow::arry,MainWindow::arry.size());
    while(1)
    {
        //if(MainWindow::bOpen)
        {
            if(bconnet)
            {
              int size = currentClient->write(Narry);
                 qDebug() << size;
               // MainWindow::currentClient->waitForBytesWritten(5);
                while(size < MainWindow::arry.size() )
                {
                  MainWindow::currentClient->waitForBytesWritten(5);
                  size += MainWindow::currentClient->write(  MainWindow::arry.right(MainWindow::arry.size() - size )  );

                }
                //MainWindow::currentClient->waitForBytesWritten(5);
               //size = size+1;
            }

        }


        qDebug() << "开始执行线程";
        QThread::sleep(1);
        qDebug() << "线程结束";
    }


}
*/
