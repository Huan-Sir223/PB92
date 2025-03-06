#ifndef PARSEDATA_H
#define PARSEDATA_H

#include <QObject>

#include <QTcpServer>
#include <QTcpSocket>

class ParseData : public QObject
{
    Q_OBJECT
public:
    explicit ParseData(QObject *parent = 0);
    ~ParseData();
    QTcpServer *tcpServer;
     QTcpSocket *currentClient;
     QByteArray arry;
     bool bconnet;
     char *buff;
public slots:

    void NewConnectionSlot();
signals:

public slots:
    void onDataChanged();

};

#endif // PARSEDATA_H
