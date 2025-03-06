#include "felfouttime.h"
#include "ui_felfouttime.h"


#include <QDebug>
FelfOutTime *FelfOutTime::insantce = nullptr;
FelfOutTime::FelfOutTime(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FelfOutTime)
{
    ui->setupUi(this);

    selftime = new QTimer(this);
    connect(this->selftime,SIGNAL(timeout()),this,SLOT(selfOutTime()));
    floatout = 5000;
}

FelfOutTime::~FelfOutTime()
{
    delete ui;
}

void FelfOutTime::selfOutTime()
{
    floatout--;
    if(floatout > 0)
    {
        ui->label->setText(QString("自毁模式:"));
        ui->label_2->setStyleSheet("QLabel { color : red; }");
        ui->label_2->setText(QString("%1.%2 S").arg(floatout/1000).arg(floatout%1000));
    }
    else {
        floatout = 5000;
        emit EndTime();
        ui->label->setText(QString("电源关闭,"));
        ui->label_2->setStyleSheet("QLabel { color : black; }");
        ui->label_2->setText(QString("自毁完成"));

        selftime->stop();
    }
}

void FelfOutTime::starttime()
{
    selftime->start(1);
}
