#ifndef FELFOUTTIME_H
#define FELFOUTTIME_H

#include <QWidget>



#include <QTimer>

//#include "mainwindow.h"


namespace Ui {
class FelfOutTime;
}

class FelfOutTime : public QWidget
{
    Q_OBJECT

public:
    explicit FelfOutTime(QWidget *parent = nullptr);

    static FelfOutTime *getInsantce()
    {
        if(insantce == nullptr)
        {
            insantce = new FelfOutTime();
        }
        return insantce;
    }


    ~FelfOutTime();

signals:

    void EndTime();


public slots:

     void selfOutTime();//自毁倒计时1
     void starttime();



private:
    Ui::FelfOutTime *ui;

    static FelfOutTime *insantce;

    QTimer *selftime;//自毁计时器

    int floatout = 500;


};

#endif // FELFOUTTIME_H
