#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <qdebug.h>
#include <QMessageBox>
#include <synchapi.h>
#include <windows.h>
#include <QDateTime>
#include <QColor>

#include <iostream>

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    myworkThread =new udpTheread;
    ui->pushButton_10->hide();
    setWindowTitle(tr("机载高速大容量记录仪v1.3.5"));
    ui->pushButton_13->hide();
    ui->pushButton_14->hide();
    ui->comboBox__l->hide();
    ui->but_open_1->hide();
    ui->but_open_2->hide();
    ui->btn_initcom_2->hide();
    ui->cbox_coms_2->hide();
    ui->btn_stop_cmd_2->hide();
    ui->pushButton_26->hide();
    ui->pushButton_34->hide();
    ui->pushButton_35->hide();

    GetAllCOMS();

    thread = new QThread;
    tcpserver = new MyTcpServer;
    tcpserver->moveToThread(thread);
    connect(thread, &QThread::finished, tcpserver, &QObject::deleteLater);
    connect(tcpserver, &QObject::destroyed, thread, &QThread::quit);
    connect(tcpserver, &QObject::destroyed, thread, &QThread::deleteLater);
    connect(this, &MainWindow::startWritingSignal, tcpserver, &MyTcpServer::startwrite);
    connect(this, &MainWindow::stopWritingSignal, tcpserver, &MyTcpServer::stopwrite);
    connect(this, &MainWindow::Disconnect, tcpserver, &MyTcpServer::Disconnect);
    connect(tcpserver,SIGNAL(to_str(QString,int)), this, SLOT(set_str(QString,int)));
    connect(tcpserver,SIGNAL(to_lisent(bool)), this, SLOT(set_lisent(bool)));
    thread->start();
    filenum = 54;


//    tcpServer = new QTcpServer(this);
   // connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);//光电图片数据模拟软件V0.1

//    if(!tcpServer->listen(QHostAddress::Any, 16205))
//    {
//        qDebug()<<"未开始监听";
//    }
//    else
//    {
//        qDebug()<<"开始监听";
//    }

    QShortcut *Z_shortcut = new QShortcut(QKeySequence("Ctrl+Alt+Z"), this);//快捷键z

    connect(Z_shortcut, &QShortcut::activated, this, &MainWindow::handleShortcut_Z);

    QShortcut *X_shortcut = new QShortcut(QKeySequence("Ctrl+Alt+C"), this);
    connect(X_shortcut, &QShortcut::activated, this, &MainWindow::handleShortcut_X);




    bOpenFile = false;
    QString nn = QCoreApplication::applicationDirPath();
    filePath =nn + "/1.bmp";
    QByteArray fileData;
    int filesize;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug()<<"文件打开失败！";
    }else
    {
        bOpenFile = true;
        QByteArray filetodata;
        filetodata = file.readAll();
        fileData = filetodata.mid(filenum,file.size()-filenum);
        filesize = fileData.size();
        file.close();
    }

    QByteArray headdata;
    QByteArray enddata;

    headdata.resize(63);
    headdata[0] = 0xe5;
    headdata[1] = 0x30;
    headdata[2] = 0x03;
    headdata[3] = 0xe2;
    headdata[4] = 0x00;
    headdata[5] = 0x23;

    int size =filesize + 53;
    headdata[6]=size&0xff;
    headdata[7]=size>>8&0xff;
    headdata[8]=size>>16&0xff;
    headdata[9]=size>>24&0xff;

    for(int i=0;i<53;i++)
    {
        headdata[10+i]=i;
    }

    enddata.resize(4);
    enddata[0] = 0x88;
    enddata[1] = 0xcc;
    enddata[2] = 0xdd;
    enddata[3] = 0xee;

    arry = headdata;
    arry += fileData;
    arry += enddata;

    tcpserver->set_data(arry);



    closeARM = NULL;
    my_serialport = NULL;
    my_serialport2 = NULL;
    IsStopCom = true;
    IsStopCom2 = true;
    IsTreeExpand = false;//tree初始状态为,不展开

    m_spSt = 0;
    m_iFramCnt = 0;
    m_iRecvFramCnt = 0;
    m_bySendBigCmd = 0;
    m_bySendSmallCmd = 0;
    m_bySendconstatus = 0;
    i_iaa55 = 0;
    m_sendRS422 = false;
    frame_num = 0;
    rs422_frame_num = 0;

    memset(m_byComcmd,0,sizeof(m_byComcmd));

    TM_sendData = new QTimer(this);

    Net_sendData = new QTimer(this);

    //    ui->cbox_reset_cmd->addItem("1.复位数管板");
    //    ui->cbox_reset_cmd->addItem("2.复位压缩板");
    //    ui->cbox_reset_cmd->addItem("3.复位两个板子");

    QStringList str;
    str << "4M" << "8M" << "12M" << "16M" << "10M" << "11M";
    ui->comboBox->addItems(str);
    QStringList sts;
    sts << "25" << "30"<< "50"<<"60";
    ui->comboBox_2->addItems(sts);

    m_bIsMyTime = false;//默认使用系统时间

    SYSTEMTIME systime;
    GetLocalTime(&systime);

    ui->le_year->setText(QString::number(systime.wYear));
    ui->le_yue->setText(QString::number(systime.wMonth));
    ui->le_ri->setText(QString::number(systime.wDay));
    ui->le_shi->setText(QString::number(systime.wHour));
    ui->le_fen->setText(QString::number(systime.wMinute));
    ui->le_miao->setText(QString::number(systime.wSecond));
    ui->le_haomiao->setText(QString::number(systime.wMilliseconds));

    InitParaName();

    //显示版本信息
    QLabel *permanent=new QLabel(this);
    //permanent->setFrameStyle(QFrame::Box|QFrame::Sunken);
    permanent->setText(tr("v1.05 2020-11-9"));
    ui->statusBar->addPermanentWidget(permanent);//显示永久信息

    //绑定信号和槽

    RS_sendData = new QTimer(this);
    // RS_sendData->setSingleShot(true);


    connect(this->RS_sendData,SIGNAL(timeout()),this,SLOT(RSSendData()));
    connect(this->Net_sendData,SIGNAL(timeout()),this,SLOT(NetSendData()));
    //connect(this->RS_sendData,SIGNAL(timeout()),this,SLOT(SendRemoteData()));

    connect(this->TM_sendData,SIGNAL(timeout()),this,SLOT(SendComDta()));

    connect(ui->btn_initcom, SIGNAL(clicked(bool)), this, SLOT(InitCOMS()));//初始化

    connect(ui->btn_stop_cmd,SIGNAL(clicked(bool)),this,SLOT(On_stop_cmd()));

    connect(ui->btn_zhijian,SIGNAL(clicked(bool)),this,SLOT(On_zhijian()));

    //大区
    connect(ui->btn_s_HDSDI,SIGNAL(clicked(bool)),this,SLOT(On_s_HDSDI()));
    connect(ui->btn_s_PAL,SIGNAL(clicked(bool)),this,SLOT(On_s_PAL()));
    connect(ui->btn_s_link1,SIGNAL(clicked(bool)),this,SLOT(On_s_Link1()));
    //connect(ui->btn_s_link2,SIGNAL(clicked(bool)),this,SLOT(On_s_Link2()));
    connect(ui->btn_s_422_1,SIGNAL(clicked(bool)),this,SLOT(On_s_422_1()));
    //connect(ui->btn_s_422_2,SIGNAL(clicked(bool)),this,SLOT(On_s_422_2()));
    //connect(ui->btn_s_422_3,SIGNAL(clicked(bool)),this,SLOT(On_s_422_3()));
    connect(ui->btn_s_ALL,SIGNAL(clicked(bool)),this,SLOT(On_s_ALL()));

    connect(ui->btn_e_HDSDI,SIGNAL(clicked(bool)),this,SLOT(On_e_HDSDI()));
    connect(ui->btn_e_PAL,SIGNAL(clicked(bool)),this,SLOT(On_e_PAL()));
    connect(ui->btn_e_link1,SIGNAL(clicked(bool)),this,SLOT(On_e_Link1()));
    //connect(ui->btn_e_link2,SIGNAL(clicked(bool)),this,SLOT(On_e_Link2()));
    connect(ui->btn_e_422_1,SIGNAL(clicked(bool)),this,SLOT(On_e_422_1()));
    //connect(ui->btn_e_422_2,SIGNAL(clicked(bool)),this,SLOT(On_e_422_2()));
    //connect(ui->btn_e_422_3,SIGNAL(clicked(bool)),this,SLOT(On_e_422_3()));
    connect(ui->btn_e_ALL,SIGNAL(clicked(bool)),this,SLOT(On_e_ALL()));

    connect(ui->pushButton,SIGNAL(clicked(bool)),this,SLOT(On_s_HDSDI2()));
    connect(ui->pushButton_2,SIGNAL(clicked(bool)),this,SLOT(On_e_HDSDI2()));
    connect(ui->pushButton_3,SIGNAL(clicked(bool)),this,SLOT(On_s_HDSDI3()));
    connect(ui->pushButton_4,SIGNAL(clicked(bool)),this,SLOT(On_e_HDSDI3()));
    connect(ui->pushButton_5,SIGNAL(clicked(bool)),this,SLOT(On_s_Asy422_1()));
    connect(ui->pushButton_6,SIGNAL(clicked(bool)),this,SLOT(On_e_Asy422_1()));
    connect(ui->pushButton_7,SIGNAL(clicked(bool)),this,SLOT(On_s_Asy422_2()));
    connect(ui->pushButton_8,SIGNAL(clicked(bool)),this,SLOT(On_e_Asy422_2()));
    connect(ui->pushButton_9,SIGNAL(clicked(bool)),this,SLOT(On_DataDel()));
    connect(ui->pushButton_10,SIGNAL(clicked(bool)),this,SLOT(On_SendRS422()));
    connect(ui->pushButton_11,SIGNAL(clicked(bool)),this,SLOT(On_Sdi1Set()));
    connect(ui->pushButton_12,SIGNAL(clicked(bool)),this,SLOT(On_sdi2Set()));
    connect(ui->pushButton_13,SIGNAL(clicked(bool)),this,SLOT(On_sdi3Set()));
    connect(ui->pushButton_14,SIGNAL(clicked(bool)),this,SLOT(On_sdi4Set()));
    connect(ui->pushButton_15,SIGNAL(clicked(bool)),this,SLOT(On_PcieReset()));
    connect(ui->pushButton_16,SIGNAL(clicked(bool)),this,SLOT(On_Restoration()));


    connect(ui->pushButton_25,SIGNAL(clicked(bool)),this,SLOT(On_s_data1()));
    connect(ui->pushButton_27,SIGNAL(clicked(bool)),this,SLOT(On_e_data1()));
    connect(ui->pushButton_28,SIGNAL(clicked(bool)),this,SLOT(On_s_data2()));
    connect(ui->pushButton_29,SIGNAL(clicked(bool)),this,SLOT(On_e_data2()));
    connect(ui->pushButton_30,SIGNAL(clicked(bool)),this,SLOT(On_s_data3()));
    connect(ui->pushButton_31,SIGNAL(clicked(bool)),this,SLOT(On_e_data3()));
    connect(ui->pushButton_32,SIGNAL(clicked(bool)),this,SLOT(On_s_data4()));
    connect(ui->pushButton_33,SIGNAL(clicked(bool)),this,SLOT(On_e_data4()));
    connect(ui->pushButton_34,SIGNAL(clicked(bool)),this,SLOT(On_s_net()));
    connect(ui->pushButton_35,SIGNAL(clicked(bool)),this,SLOT(On_e_net()));
    connect(ui->speedTestPushButton, SIGNAL(clicked(bool)),this,SLOT(On_WriteSpeedTest()));





    //ARM

    connect(ui->btn_tree,SIGNAL(clicked(bool)),this,SLOT(On_Tree_expand()));

    //connect(ui->btn_ARM_connect,SIGNAL(clicked(bool)),this,SLOT(On_connect()));


    // connect(ui->btn_ARM_reset,SIGNAL(clicked(bool)),this,SLOT(On_ResetUnit()));
    //connect(ui->btn_ARM_format,SIGNAL(clicked(bool)),this,SLOT(On_FormatStorage()));
    connect(ui->btn_ARM_write,SIGNAL(clicked(bool)),this,SLOT(On_WriteRegister()));
    connect(ui->btn_ARM_read,SIGNAL(clicked(bool)),this,SLOT(On_ReadRegister()));

    connect(this,SIGNAL(ParseData_AA55(QByteArray)),this,SLOT(To_ParseData_AA55(QByteArray)));
    connect(this,SIGNAL(ParseData_B5A9(QByteArray)),this,SLOT(To_ParseData_B5A9_PB85(QByteArray)));
    connect(this,SIGNAL(ParseData_C7A7(QByteArray)),this,SLOT(To_ParseData_C7A7(QByteArray)));

    // connect(this,SIGNAL(Do(QByteArray)),this,SLOT(ToDO(QByteArray)));

    connect(ui->rbtn,SIGNAL(clicked(bool)),this,SLOT(slot_rbtn_time()));

    emit ui->btn_tree->clicked(true);

    QString st("C7 A7 12 26 00 00 58 02 23 7F 7F 42 42 42 11 00 01 B9");
    QStringList sli = st.split(" ");
    for(int i = 0; i < sli.size();i++)
    {
        tempData.append(sli[i].toInt(0, 16));
    }
    startData = uchar(0x7F);

    ui->pushButton_10->setEnabled(false);
    //    ui->pushButton_18->setEnabled(false);
    ui->pushButton_21->setEnabled(false);
    bListen = false;


    camera_s = new QTimer(this);

    connect(this->camera_s,SIGNAL(timeout()),this,SLOT(camera_so()));

    camera_s->start(1000);

    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();   // 获取可用摄像头列表
    for(auto camera : cameras)
    {
        //  ui->com_cameras->addItem(camera.description());
        ui->comboBox__l->addItem(camera.description());

    }


    //创建对象 初始化
    //udpSocket = new QUdpSocket();
    //绑定
    // udpSocket->bind(QHostAddress::AnyIPv4,8080);
    //关联读数据信号readyread
    //connect(udpSocket,SIGNAL(readyRead()),this,SLOT(read_data()));
    connect(ui->comboBox_3, SIGNAL(currentTextChanged(QString)), this, SLOT(get_text()));//压缩率变化


    //udp文件存储
    // myworkThread->start();
    bool ok = connect(this,SIGNAL(datafile(QString)),myworkThread,SLOT(get_load(QString)));
    qDebug()<<"dadad"<<ok;

    srcDirPath=QCoreApplication::applicationDirPath()+"/data/";
    qDebug()<<srcDirPath;
    ui->label_bc->setText(srcDirPath);
    emit datafile(srcDirPath);
    ui->label_bc->hide();
    ui->pushButton_bc->hide();//隐藏多余按钮
}
void MainWindow::set_lisent(bool ok)
{

    if(ok)
    {
        if(ui->radioButton->isChecked())
        {
            emit startWritingSignal();
        }
        ui->pushButton_21->setEnabled(true);
    }
    else
    {
        ui->pushButton_21->setEnabled(false);
    }
}
void MainWindow::ToDo(QByteArray)
{


}
unsigned int calccrc(unsigned char crcbuf, unsigned int crc)
{
    unsigned char i;

    crc = crc ^ crcbuf;

    for (i = 0; i < 8; i++)
    {
        unsigned char chk;

        chk = crc & 1;

        crc = crc >> 1;

        crc = crc & 0x7fff;

        if (chk == 1)
            crc = crc ^ 0xa001;

        crc = crc & 0xffff;
    }

    return crc;
}
unsigned int chkcrc(unsigned char *buf, unsigned char len)
{
    unsigned char hi, lo;

    unsigned int i;

    unsigned int crc;

    crc = 0xFFFF;

    for (i = 0; i < len; i++)
    {
        crc = calccrc(*buf, crc);

        buf++;
    }

    hi = crc % 256;

    lo = crc / 256;

    crc = (hi << 8) | lo;

    return crc;
}
MainWindow::~MainWindow()
{
    //file.close();
    delete ui;
    if(closeARM != NULL)
    {
        delete closeARM;
        closeARM = NULL;
    }

}
void udpTheread::run()
{
    qDebug()<<"thread open";
    udpSocket_t = new QUdpSocket(this);
    udpSocket_t->bind(QHostAddress::Any,8080);
    QByteArray aaa;
    aaa[0]=0xeb;
    aaa[1]=0x90;
    aaa[2]=0xa0;
    aaa[3]=0x00;
    aaa[4]=0x00;
    aaa[5]=0x7b;

    udpSocket_t->writeDatagram(aaa,aaa.size(),QHostAddress("100.1.1.187"),8080);



    while(1)
    {


        while(udpSocket_t->hasPendingDatagrams())
        {
            QByteArray arrz;
            arrz.resize(udpSocket_t->pendingDatagramSize());
            int size=udpSocket_t->readDatagram(arrz.data(),arrz.size());
            // qDebug()<<size;
            if(size!=1044)
            {
                qDebug()<<"erro size";
                continue;

            }
            QByteArray arr;
            QByteArray arrd;
            arrd.resize(20);
            arr.resize(1024);
            for(int i =0;i<20;i++)
            {
                arrd[i]=arrz[i];
            }
            for(int i =20;i<1044;i++)
            {
                arr[i-20]=arrz[i];
            }
            if((u_char)(arrz.at(1))==0x11)
            {
                bzw=0;

            }
            else
            {
                bzw=1;
            }
            if(bzw==0)
            {
                if((u_char)(arr.at(0))==0xeb&&(u_char)(arr.at(1))==0x90&&(u_char)(arr.at(2))==0x00&&(u_char)(arr.at(3))==0x04)
                {
                    //QByteArray abcde=arr.mid(4);

                    uint32_t num_1;
                    uint32_t num_2;
                    for(int i=3;i<1024/4-1;i+=2)

                    {
                        num_1=((u_char)(arr.at(4*i+0))<<24)|((u_char)(arr.at(4*i+1))<<16)|((u_char)(arr.at(4*i+2))<<8)|((u_char)(arr.at(4*i+3)));
                        num_2=((u_char)(arr.at(4*i+4))<<24)|((u_char)(arr.at(4*i+5))<<16)|((u_char)(arr.at(4*i+6))<<8)|((u_char)(arr.at(4*i+7)));
                        //  qDebug()<<num_1<<num_2;
                        if((num_1+1)!=num_2)
                        {
                            qDebug()<<"erro1 jy"<<num_1<<i;
                            if(num_1==0xffffffff)
                            {
                                continue;
                            }
                            else
                            {
                                qDebug()<<"erro2 jy"<<i;
                                //  qDebug()<<arr.toHex(' ');
                                break;
                            }
                        }

                    }

                    //  qDebug()<<"jy success";

                    //  qDebug()<<arr.toHex(' ');
                    filedata.append(arr);
                    if(filedata.size()>1024*1024*25)
                    {
                        get_data();
                        qDebug()<<"over";
                        filedata.clear();

                    }

                }


                /* if((u_char)(arr.at(0))==0xeb&&(u_char)(arr.at(1))==0x90&&(u_char)(arr.at(2))==0x00&&(u_char)(arr.at(3))==0x04)
                                {
                    unsigned char pCharData1[3000] = { 0x00 };
                    for (int i=0;i<1022;i++)
                    {
                        pCharData1[i] = arr.at(i);
                    }
                    uint16_t crc16;

                    for(int i=0;i<1022;i++)
                    {
                        crc16=crc16+pCharData1[i];
                    }
                    //qDebug()<<crc16;
                    QByteArray jy1;
                    jy1[0]=crc16>>8;
                    QByteArray jy2;
                    jy2[0]=crc16;
                   // qDebug()<<jy1.toHex()<<jy2.toHex();
                    if(jy1[0]!=arr[1023]||jy2[0]!=arr[1022])
                    {
                        QByteArray ab1;
                        QByteArray ab2;

                        ab1[0]=arr[8];
                        ab2[0]=arr[9];
                        QByteArray ac1;
                        QByteArray ac2;

                        ac1[0]=arr[1022];

ac2[0]=arr[1023];
                        int nu_xu=MAKEWORD(arr[8],arr[9]);
                       qDebug()<<"jy erro"<<jy1.toHex()<<jy2.toHex()<<ac1.toHex()<<ac2.toHex()<<nu_xu;
                       qDebug()<<arr.toHex(' ');
                      // return;
                    }
                    crc16=0;



                                    if((u_char)(arr.at(10))==0x13)
                                    {
                                     qDebug()<<arr.toHex(' ');
                                    /* unsigned char pCharData1[3000] = { 0x00 };
                                     for (int i=0;i<1022;i++)
                                     {
                                         pCharData1[i] = arr.at(i);
                                     }*/
                /*uint16_t crc16;

                                      for(int i=0;i<1022;i++)
                                      {
                                          crc16=crc16+pCharData1[i];
                                      }
                                      qDebug()<<crc16;
                                       QByteArray jy1;
                                       jy1[0]=crc16>>8;
                                       QByteArray jy2;
                                       jy2[0]=crc16;
                                       qDebug()<<jy1.toHex()<<jy2.toHex();
                                        crc16=0;



                                        int a1=(arr.at(67)<<24)&0xff000000;
                                        int a2=(arr.at(66)<<16)&0x00ff0000;
                                        int a3=(arr.at(65)<<8)&0x0000ff00;
                                        int a4=(arr.at(64))&0x000000ff;
                                        QByteArray abc1;
                                        QByteArray abc2;
                                        QByteArray abc3;
                                        QByteArray abc4;
                                        abc1[0]=arr[64];
                                        abc2[0]=arr[65];
                                        abc3[0]=arr[66];
                                        abc4[0]=arr[67];
                                        data_num=a1|a2|a3|a4;
                                        //  qDebug()<<abc1.toHex()<<abc2.toHex()<<abc3.toHex()<<abc4.toHex();
                                        qDebug()<<(quint32)(a1|a2|a3|a4);
                                        filedata.clear();
                                        QByteArray lin_data;
                                        lin_data=arr.mid(11,53);
                                        filedata.append(lin_data);
                                        get_data();
                                        i_b=1;
                                    }
                                    else if(i_b==1&&(u_char)(arr.at(10))==0x15)
                                    {
                                        QByteArray ab1;
                                        QByteArray ab2;

                                        ab1[0]=arr[8];
                                        ab2[0]=arr[9];

                                        int nu_xu=MAKEWORD(arr[8],arr[9]);

                                        if(nu_xu==1)
                                        {
                                            qDebug()<<"head";
                                            filedata.clear();

                                        }

                                        if((z_num+1)==nu_xu){

                                            z_num = nu_xu;
                                            //qDebug()<<z_num<<data_num;
                                            if(data_num>1011)
                                            {
                                                QByteArray lin_data;
                                                lin_data=arr.mid(11,1011);
                                                filedata.append(lin_data);
                                                data_num=data_num-1011;
                                            }
                                            else
                                            {
                                                qDebug()<<"end:"<<data_num;
                                                QByteArray lin_data;
                                                lin_data=arr.mid(11,data_num);
                                                filedata.append(lin_data);
                                                data_num=0;
                                                z_num=0;
                                                qDebug()<<"wei:"<<filedata.size();
                                                i_b=0;
                                                get_data();
                                                filedata.clear();
                                            }

                                        }
                                        else
                                        {
                                            qDebug()<<"zhen erro:"<<z_num<<nu_xu<<data_num;
                                            //return;
                                            data_num=0;
                                            z_num=0;
                                            i_b=0;
                                        }
                                    }
                                }*/



                /*if((u_char)(arr.at(0))==0xeb&&(u_char)(arr.at(1))==0x90&&(u_char)(arr.at(2))==0x00&&(u_char)(arr.at(3))==0x04)
                {





                    if((u_char)(arr.at(10))==0x13)
                    {
                     qDebug()<<arr.toHex(' ');
                     unsigned char pCharData1[3000] = { 0x00 };
                     for (int i=0;i<1022;i++)
                     {
                         pCharData1[i] = arr.at(i);
                     }
                      uint16_t crc16;

                      for(int i=0;i<1022;i++)
                      {
                          crc16=crc16+pCharData1[i];
                      }
                      qDebug()<<crc16;

                        int a1=(arr.at(67)<<24)&0xff000000;
                        int a2=(arr.at(66)<<16)&0x00ff0000;
                        int a3=(arr.at(65)<<8)&0x0000ff00;
                        int a4=(arr.at(64))&0x000000ff;
                        QByteArray abc1;
                        QByteArray abc2;
                        QByteArray abc3;
                        QByteArray abc4;
                        abc1[0]=arr[64];
                        abc2[0]=arr[65];
                        abc3[0]=arr[66];
                        abc4[0]=arr[67];
                        data_num=a1|a2|a3|a4;
                        //  qDebug()<<abc1.toHex()<<abc2.toHex()<<abc3.toHex()<<abc4.toHex();
                        qDebug()<<(quint32)(a1|a2|a3|a4);
                        filedata.clear();
                        QByteArray lin_data;
                        lin_data=arr.mid(11,53);
                        filedata.append(lin_data);
                        get_data();
                        i_b=1;
                    }
                    else if(i_b==1&&(u_char)(arr.at(10))==0x15)
                    {
                        QByteArray ab1;
                        QByteArray ab2;

                        ab1[0]=arr[8];
                        ab2[0]=arr[9];

                        int nu_xu=MAKEWORD(arr[8],arr[9]);

                        if(nu_xu==1)
                        {
                            qDebug()<<"head";
                            filedata.clear();

                        }

                        if((z_num+1)==nu_xu){

                            z_num = nu_xu;
                            //qDebug()<<z_num<<data_num;
                            if(data_num>1011)
                            {
                                QByteArray lin_data;
                                lin_data=arr.mid(11,1011);
                                filedata.append(lin_data);
                                data_num=data_num-1011;
                            }
                            else
                            {
                                qDebug()<<"end:"<<data_num;
                                QByteArray lin_data;
                                lin_data=arr.mid(11,data_num);
                                filedata.append(lin_data);
                                data_num=0;
                                z_num=0;
                                qDebug()<<"wei:"<<filedata.size();
                                i_b=0;
                                get_data();
                                filedata.clear();
                            }

                        }
                        else
                        {
                            qDebug()<<"zhen erro:"<<z_num<<nu_xu<<data_num;
                            //return;
                            data_num=0;
                            z_num=0;
                            i_b=0;
                        }
                    }
                }*/
            }





        }
    }
}




void udpTheread::get_load(QString str)
{
    srcload=str;
    qDebug()<<"thread:"<<str;
}
void udpTheread::get_data()
{

    //  dataarry = arry;
    // srcload =str;
    //data_run=true;
    //qDebug()<<"arry"<<dataarry.size()<<data_run;
    // srcload="D:/test/build-MyQT_Register-Desktop_Qt_5_9_4_MinGW_32bit-Release/release/data/";
    time = QDateTime::currentDateTime();
    //qDebug() << time.toString("yyyyMMddhhmmss");
    QString name;
    if(filedata.length()>60)
    {
        name = srcload+time.toString("yyyyMMddhhmmss")+".j2k";//j2k
        qDebug() <<name;
        file.setFileName(name);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            qDebug()<< "Open failed." ;

            //return ;
            return;
        }
        file.write(filedata,filedata.length());//这种方式也不会有多余字节
        file.close();
        delete_data();
    }
    else
    {
        name = srcload+time.toString("yyyyMMddhhmmss")+".txt";//txt
        qDebug() <<name;
        // QString tox=filedata.toHex(' ');
        file.setFileName(name);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            qDebug()<< "Open failed." ;

            //return ;
            return;
        }
        file.write(filedata.toHex(' '));//这种方式也不会有多余字节
        file.close();
        delete_data();
    }


    //data_run=false;
    // filedata.clear();
    //srcload.clear();


}
void udpTheread::delete_data()
{
    QDir dir(srcload);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);  //路径下文件，不包含子文件夹
    //NoDotAndDotDot一定要加！！不然会检测出..
    //dir.setSorting(QDir::Time); //时间排序是从最新到最早
    dir.setSorting(QDir::Time| QDir::Reversed); //按时间先后排序，从最早到最新
    QStringList fileList = dir.entryList();
    QString str;
    //qDebug()<<"num"<<fileList.size();
    if(fileList.size()>20)
    {
        //删除文件夹中文件
        for (int i = 0; i < fileList.size()-20; i++)
        {
            str= srcload  + fileList.at(i);
            // qDebug()<<"num"<<str;
            QFile removFile(str);
            removFile.remove();
        }
    }
}
void MainWindow::read_data()
{


    //  qDebug()<<" asd";
    //数据缓冲区
    QByteArray arr;
    arr.resize(udpSocket->pendingDatagramSize());
    int size=udpSocket->readDatagram(arr.data(),arr.size());


    if(size<=0)
    {
        //continue;
        //break;
        return;
    }
    // qDebug()<<arr.toHex(' ');

    if((u_char)(arr.at(0))==0xEB&&(u_char)(arr.at(1))==0x90&&(u_char)(arr.at(2))==0x04&&(u_char)(arr.at(3))==0x00)
    {
        if(tile_num==0)
        {
            tile = "数据查找";
            //ui->label_ts->setText(tile);
        }
        //qDebug()<<"buru";
        // arr.append(mSocket->readDatagram(1020));

        QString str=arr.toHex();
        if(i_b==1)
        {
            if(str.contains("88ccddee",Qt::CaseSensitive))
            {
                int m;
                int j = 0;
                while ((j = str.indexOf("88ccddee", j)) != -1) {
                    //qDebug() << "Found <b> tag at index position " << j << endl;
                    m=j;
                    ++j;
                }
                j=m/2;
                int k=(j-8);
                QByteArray cc=arr.mid(8,k);
                filedata.append(cc);
                //file.write(cc,cc.length());//这种方式也不会有多余字节
                i_b=0;
                //  bb++;
                /* QByteArray num;
                    num.resize(4);
                    num[0]=arr[4];
                    num[1]=arr[5];
                    num[2]=arr[6];
                    num[3]=arr[7];*/
                // QString num_a=num.toHex(' ');
                //qDebug()<<"tail:"<<bb<<num.toHex(' ');
                //file.close();
                // filedata.clear();
                if(data_num==filedata.size())
                {
                    qDebug()<<"ok";
                    //emit datafile(filedata,srcDirPath);
                }
                if(tile_num==1)
                {
                    tile = "找到枕尾";
                    //ui->label_ts->setText(tile);
                    // ui->label_14->setText(QString::number(filedata.size()));
                }
                // myworkThread->get_data(filedata);
                qDebug()<<"filedata:"<<filedata.size();
                tile_num=0;
                filedata.clear();
                //delete_data();
                return;

            }
            QByteArray cc=arr.mid(8,1014);
            //file.write(cc,cc.length());//这种方式也不会有多余字节
            filedata.append(cc);
            //  bb++;

            // qDebug()<<bb;

        }
        if(str.contains("66bbeeff",Qt::CaseSensitive))
        {
            filedata.clear();
            // time = QDateTime::currentDateTime();
            //qDebug() << time.toString("yyyyMMddhhmmss");
            // QString name = srcDirPath+time.toString("yyyyMMddhhmmss")+".j2k";//j2k
            // qDebug() <<name;
            //   file.setFileName(name);

            // if(!file.open(QIODevice::WriteOnly | QIODevice::Append))
            //  {
            //     qDebug()<< "Open failed." ;
            //return ;
            //     return;
            //   }
            //char *pdata; unsigned long *bfSize;
            // writeBmp(pdata, bfSize);
            int j = 0;
            int m;
            while ((j = str.indexOf("66bbeeff", j)) != -1) {
                qDebug() << "Found <b> tag at index position " << j << endl;
                m=j;
                ++j;
            }
            qDebug() << "Found <b2> tag at index position " << m << endl;

            j=m/2;
            int k=(1021-j-8)+1;
            QByteArray cc=arr.mid(j+8,k);
            // file.write(cc,cc.length());//这种方式也不会有多余字节
            filedata.append(cc);
            qDebug()<<"head:"<<filedata.size();
            i_b=1;
            //bb++;
            QByteArray num;
            num.resize(4);
            num[0]=arr[j+4];
            num[1]=arr[j+5];
            num[2]=arr[j+6];
            num[3]=arr[j+7];
            //  data_num=((num.at(3)<<24)&0xff000000)|((num.at(2)<<16)&0x00ff0000)|((num.at(1)<<8)&0x0000ff00)&&((num.at(0))&0x000000ff);
            int a1=(num.at(3)<<24)&0xff000000;
            int a2=(num.at(2)<<16)&0x00ff0000;
            int a3=(num.at(1)<<8)&0x0000ff00;
            int a4=(num.at(0))&0x000000ff;
            qDebug()<<a1<<a2<<a3<<a4;
            data_num=a1|a2|a3|a4;
            //qDebug()<<cc.toHex(' ');

            tile = "找到帧头";
            //ui->label_ts->setText(tile);
            tile_num=1;

            qDebug()<<num.toHex(' ')<<data_num;
            //qDebug()<<"head:"<<bb<<num.toHex(' ');
            // qDebug()<<j<<k;
            //return;
        }


    }
    else
    {
        //qDebug()<<"erro";
    }


}

void MainWindow::camera_so()
{
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();   // 获取可用摄像头列表
    for(auto camera : cameras)
    {
        //  ui->com_cameras->addItem(camera.description());
        ui->comboBox__l->addItem(camera.description());

    }


}


void MainWindow::NewConnectionSlot()
{
    qDebug()<<"jin!";
    currentClient = tcpServer->nextPendingConnection();
    //tcpClient.append(currentClient);
    //  currentClient->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,QVariant::fromValue(2*1024*1024));
    //currentClient->setSocketOption(QAbstractSocket::LowDelayOption,0);

    connect(currentClient, SIGNAL(readyRead()), this, SLOT(ReadData()));
    connect(currentClient, SIGNAL(disconnected()), this, SLOT(disconnectedSlot()));
    //    ui->pushButton_18->setEnabled(true);
    ui->pushButton_21->setEnabled(true);
    //currentClient->write("123");

}

void MainWindow::slot_rbtn_time()
{
    if(m_bIsMyTime)
    {
        m_bIsMyTime = false;

    }
    else
    {
        m_bIsMyTime = true;

    }
}

void MainWindow::GetAllCOMS()
{
    //读取串口信息
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        //这里相当于自动识别串口号之后添加到了cmb，如果要手动选择可以用下面列表的方式添加进去
        //QSerialPort serial;
        //serial.setPort(info);
        //if (serial.open(QIODevice::ReadWrite))
        //{
        //将串口号添加到cmb

        ui->cbox_coms->addItem(info.portName());
        ui->cbox_coms_2->addItem(info.portName());
        ui->cbox_coms_3->addItem(info.portName());

        //关闭串口等待人为(打开串口按钮)打开
        //serial.close();
        //}
    }

    //    if (ui->cbox_coms->count() < 1)
    //    {
    //        if (QMessageBox::information(this, tr("提示"), tr("没有检测到串口信息"), QMessageBox::Retry|QMessageBox::Cancel) == QMessageBox::Retry)
    //        {
    //            GetAllCOMS();
    //        }
    //        else
    //        {
    //            return;
    //        }
    //    }
}

void MainWindow::InitCOMS()
{ 
    ui->label_10->setStyleSheet("background-color: rgb(0, 255, 0)");
    if (ui->cbox_coms->count() < 1)
    {
        if (QMessageBox::information(this, tr("提示"), tr("没有设置可用的串口信息"), QMessageBox::Retry | QMessageBox::Cancel) == QMessageBox::Retry)
        {
            GetAllCOMS();
        }
        else
        {
            return;
        }
    }

    if(!IsStopCom)
    {
        //QMessageBox::information(this, tr("提示"), tr("串口通讯进行中,不支持重复初始化!"), QMessageBox::Ok);
        //关闭串口
        IsStopCom = true;
        TM_sendData->stop();
        if(RS_sendData->isActive())
            RS_sendData->stop();
        my_serialport->close();
        delete my_serialport;
        my_serialport = NULL;
        m_spSt = 0;
        frame_num = 0;
        ui->statusBar->showMessage(tr("停止%1串口通信").arg(ui->cbox_coms->currentText()),3000);
        ui->btn_initcom->setText(tr("打开串口"));
        return ;
    }
    //    struct PortSettings myComSetting = {BAUD115200,DATA_8,PAR_NONE,STOP_1,FLOW_OFF,500};
    //    struct PortSettings myComSetting = {BAUD38400,DATA_7,PAR_NONE,STOP_1,FLOW_OFF,500};
    //    my_serialport->setPortName(ui->cbox_coms->currentText());
    //    my_serialport = new Win_QextSerialPort(ui->cbox_coms->currentText(),myComSetting,QextSerialBase::EventDriven);
    my_serialport = new Win_QextSerialPort(ui->cbox_coms->currentText(),QextSerialBase::EventDriven);

    if (my_serialport->open(QIODevice::ReadWrite))
    {
        //设置波特率
        my_serialport->setBaudRate(BAUD115200);

        //设置数据位
        my_serialport->setDataBits(DATA_8);
        //设置校验位
        my_serialport->setParity(PAR_NONE);
        //设置流控制
        my_serialport->setFlowControl(FLOW_OFF);
        //设置停止位
        my_serialport->setStopBits(STOP_1);
        ui->statusBar->showMessage(tr("%1初始化成功,串口通讯已开启").arg(ui->cbox_coms->currentText()),3000);
        connect(this->my_serialport,SIGNAL(readyRead()),this,SLOT(GetComDta()));
        ui->btn_initcom->setText(tr("关闭串口"));
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("串口打开失败,请确认串口状态"), QMessageBox::Ok);
        return;
    }
    IsStopCom = false;
    ui->label_10->setStyleSheet("background-color: rgb(0, 255, 0)");
    TM_sendData->start(1000);
}

int MainWindow::hex2int(QString strData, int len)
{
    int temp = 0, iout = 0;
    for (int i = 0; i<strData.length(); ++i)
    {
        if (strData.toStdString()[i] == 'a' || strData.toStdString()[i] == 'A') {
            temp = 10;
        } else if (strData.toStdString()[i] == 'b' || strData.toStdString()[i] == 'B') {
            temp = 11;
        } else if (strData.toStdString()[i] == 'c' || strData.toStdString()[i] == 'C') {
            temp = 12;
        } else if (strData.toStdString()[i] == 'd' || strData.toStdString()[i] == 'D') {
            temp = 13;
        } else if (strData.toStdString()[i] == 'e' || strData.toStdString()[i] == 'E') {
            temp = 14;
        } else if (strData.toStdString()[i] == 'f' || strData.toStdString()[i] == 'F') {
            temp = 15;
        } else {
            temp = strData.toStdString()[i] - '0';
        }

        iout += (temp * pow(16.0, len - 1 - i));
    }
    return iout;
}

void MainWindow::SendARMcmd(int _len,unsigned char cmd)
{
    if(my_serialport == NULL)
        return;
    if(!IsStopCom) {
        QByteArray sendData;
        char g_cmd=0x00;

        if(cmd == 0x02 || cmd == 0x03 || cmd == 0x06 ||cmd == 0x05|| cmd == 0x09 || cmd == 0x0A)
        {
            sendData.append(0xAA);
            sendData.append(0x04);
            sendData.append(cmd);

            for(int i = 0; i < 3 ; i++)
            {
                g_cmd += sendData.at(i);
            }
            sendData.append(g_cmd);

            while(sendData.length() < 16)
                sendData.append((char)0x00);

            my_serialport->write(sendData);
        }

        if(cmd == 0x04)
        {
            sendData.append(0xAA);
            sendData.append(0x04);
            sendData.append(cmd);

            for(int i = 0; i < 3 ; i ++)
            {
                g_cmd += sendData.at(i);
            }
            sendData.append(g_cmd);

            while(sendData.length() < 16)
                sendData.append((char)0x00);

            my_serialport->write(sendData);
        }

        //        if(cmd == 0x05)
        //        {
        //            sendData.append(0xAA);
        //            sendData.append(_len+4);
        //            sendData.append(cmd);

        //            if(ui->cbox_coms->currentText() == "1.复位数管板")
        //            {
        //                sendData.append(0x01);
        //            }
        //            else if(ui->cbox_coms->currentText() == "2.复位压缩板")
        //            {
        //                sendData.append(0x02);
        //            }
        //            else if(ui->cbox_coms->currentText() == "3.复位两个板子")
        //            {
        //                sendData.append(0x03);
        //            }

        //            for(int i = 0; i < 4 ; i ++)
        //            {
        //                g_cmd += sendData.at(i);
        //            }
        //            sendData.append(g_cmd);

        //            while(sendData.length() < 16)
        //                sendData.append((char)0x00);

        //            my_serialport->write(sendData);
        //        }

    }
    else
    {
        QMessageBox::about(this,"提示","串口未初始化，无法发送指令");
    }
}
void MainWindow::get_text()
{
    qDebug()<<"索引："<<ui->comboBox_3->currentIndex()<<"内容："<<ui->comboBox_3->currentText();
    int num = ui->comboBox_3->currentIndex();
    if(num==0)
    {
        m_bySendICR=0x1;
        qDebug()<<"m_bySendICR:"<<m_bySendICR;
    }
    else if(num==1)
    {
        m_bySendICR=0x2;
        qDebug()<<"m_bySendICR:"<<m_bySendICR;
    }
    else if(num==2)
    {
        m_bySendICR=0x3;
        qDebug()<<"m_bySendICR:"<<m_bySendICR;
    }
    else if(num==3)
    {
        m_bySendICR=0x4;
        qDebug()<<"m_bySendICR:"<<m_bySendICR;
    }
    m_bySendBigCmd = 0x63;
    SendRemoteData();
    ui->statusBar->showMessage(tr("设置光电数据传输速率"));

}
void MainWindow::SendRemoteData()
{
    if(my_serialport == NULL)
        return;
    QByteArray sendData;

    unsigned char bytime[8]={0x00};
    unsigned char bycmd[16] = {0x00};
    bycmd[0] = 0xc7;
    bycmd[1] = 0xa7;
    bycmd[2] = 0x10;
    bycmd[4] = GetSysTime2Byte(bytime)[0];
    bycmd[3] = GetSysTime2Byte(bytime)[1];
    bycmd[5] = GetSysTime2Byte(bytime)[2];
    bycmd[6] = GetSysTime2Byte(bytime)[3];
    bycmd[7] = GetSysTime2Byte(bytime)[4];
    bycmd[8] = GetSysTime2Byte(bytime)[5];
    bycmd[9] = GetSysTime2Byte(bytime)[6];
    bycmd[10] = GetSysTime2Byte(bytime)[7];
    bycmd[11] = m_bySendBigCmd; ///大区控制指令
    bycmd[13] = m_bySendICR;
    /// 版本1,0
    //        bycmd[11] = m_bySendSmallCmd;   ///小区控制指令
    //        bycmd[12] = m_bySendconstatus;  /// 发送状态

    /// 版本2.0 时间：2018-12-28 修改内容：删除小区控制指令及指令状态指令
    ///

    /// 版本3.0 时间：2019-09-06 修改内容：参照滕盾20190906新协议
    //bycmd[14] = (unsigned char)m_iFramCnt++;
    if(m_bySendBigCmd == 0x41
            || m_bySendBigCmd == 0x42
            || m_bySendBigCmd == 0x43
            || m_bySendBigCmd == 0x44)
    {
        char rate;
        char gop;
        int rate_index = ui->comboBox->currentIndex();
        int gop_index = ui->comboBox_2->currentIndex();

        switch(rate_index)
        {
        case 0: rate = 0x1;break;
        case 1: rate = 0x2;break;
        case 2: rate = 0x4;break;
        case 3: rate = 0x8;break;
        case 4: rate = 0x5;break;
        case 5: rate = 0x6;break;
        default: rate = 0x0;break;
        }
        switch(gop_index)
        {
        case 0: gop = 0x1;break;
        case 1: gop = 0x2;break;
        case 2: gop = 0x4;break;
        case 3: gop = 0x8;break;
        default: gop = 0x0;break;
        }
        char val = (gop << 4)  |  rate;
        bycmd[12] = val;
    }
    else
    {
        bycmd[12] = 0x0;
    }
    if(m_bySendBigCmd = 0x63)
    {
        char rate;
        int num = ui->comboBox_3->currentIndex();
        switch(num)
        {
        case 0: rate = 0x1;break;
        case 1: rate = 0x2;break;
        case 2: rate = 0x3;break;
        case 3: rate = 0x4;break;
            bycmd[14] = rate;
        }
    }
    int iCheckCode = 0;
    for (int i = 0; i < 15; i++)
    {
        iCheckCode += bycmd[i];
        sendData.append(bycmd[i]);
    }
    bycmd[15] = (unsigned char)(iCheckCode & 0xff);

    sendData.append(bycmd[15]);
    selfdata = sendData;
    //my_serialport->readAll();
    my_serialport->write(sendData);


    qDebug() << "sendData:"<<sendData.toHex(' ');

    //    QDateTime current_date_time = QDateTime::currentDateTime();
    //    QString current_time = current_date_time.toString("hh:mm:ss.zzz ");
    //    qDebug() << "sendRemotetime:"<<current_time;
    m_bySendBigCmd = 0;
}
void MainWindow::CloseARMStatus()
{
    CloseStatus = false;
}
void MainWindow::NetSendData()
{
    if(QAbstractSocket::ConnectedState == currentClient->state())
    {
        currentClient->write(arry);
        currentClient->flush();
        currentClient->waitForBytesWritten();
        QCoreApplication::processEvents();
    }

}
void MainWindow::RSSendData()
{
    //     if(my_serialport == NULL)
    //         return;
    //     QByteArray sendData;
    //     unsigned char rscmd[512] = {0x00};
    //     //RS422载荷数据
    //     rscmd[0] = 0xEE;
    //     rscmd[1] = 0x16;
    //     short n = 512;
    //     char p[2];
    //     memcpy(p, (char*)&n, sizeof(short));
    //     rscmd[2] = p[0];
    //     rscmd[3] = p[1];

    //     char num[2];
    //     memcpy(num, (char*)&rs422_frame_num, sizeof(unsigned short));
    //     rscmd[4] = num[0];
    //     rscmd[5] = num[1];

    //     for(int i= 0; i < 505;i++)
    //     {
    //         rscmd[6+i] = (unsigned char)i;
    //     }
    //     int iCheckCode = 0;
    //     for (int i = 0; i < 511; i++)
    //     {
    //         iCheckCode += rscmd[i];
    //         sendData.append(rscmd[i]);
    //     }
    //     rscmd[511] = (unsigned char)(iCheckCode & 0xff);
    //     sendData.append(rscmd[511]);
    //     //my_serialport->readAll();
    //     my_serialport->write(sendData);
    //     rs422_frame_num++;

    if(my_serialport2 == NULL)
    {
        return;
    }

    QByteArray sendData;
    unsigned char rscmd[235] = {0x00};
    //RS422载荷数据
    rscmd[0] = 0xEE;
    rscmd[1] = 0x16;
    //     short n = 235;
    //     char p[2];
    //     memcpy(p, (char*)&n, sizeof(short));
    //rscmd[2] = p[0];
    rscmd[2] = 0;
    rscmd[3] = 235;

    char num[2];
    memcpy(num, (char*)&rs422_frame_num, sizeof(unsigned short));
    rscmd[4] = num[0];
    rscmd[5] = num[1];

    for(int i= 0; i < 229;i++)
    {
        rscmd[6+i] = (unsigned char)i;
    }
    //带时标数据帧
    unsigned char tframe[64] = {0x00};
    tframe[0] = 0xEB;
    tframe[1] = 0x90;
    tframe[2] = 0x40;
    tframe[3] = 0x45;
    //
    QDateTime current_data_time = QDateTime::currentDateTime();
    int year = current_data_time.date().year();
    int month = current_data_time.date().month();
    int day = current_data_time.date().day();
    int yy = year - 2000;
    unsigned short dt = 0;
    dt |= yy;
    dt |= month << 7;
    dt |= day << 11;
    char ymd[2];
    memcpy(ymd, (char*)&dt, sizeof(unsigned short));
    tframe[38] = ymd[0];
    tframe[39] = ymd[1];

    QTime ct = QTime::currentTime();
    int hour = ct.hour();
    if(hour < 8)
        hour =hour + 24;
    hour = hour - 8;
    int minute = ct.minute();
    int second = ct.second();
    int msec = ct.msec();
    unsigned int val = hour * 60 * 60 * 1000 + minute * 60 * 1000 + second * 1000 + msec;
    // qDebug()<<"年："<<year<<month<<day<<
    char ms[4];
    memcpy(ms, (char*)&val, sizeof(unsigned int));
    tframe[48] = ms[0];
    tframe[49] = ms[1];
    tframe[50] = ms[2];
    tframe[51] = ms[3];
    int iCheckCode = 0;
    for (int i = 0; i < 63; i++)
    {
        iCheckCode += tframe[i];
    }
    tframe[63] = (unsigned char)(iCheckCode & 0xff);

    for(int i = 0 ; i < 64; i++)
    {
        rscmd[20+i] = tframe[i];
    }
    //int iCheckCode = 0;
    for (int i = 0; i < 235; i++)
    {
        //iCheckCode += rscmd[i];
        sendData.append(rscmd[i]);
    }
    //rscmd[511] = (unsigned char)(iCheckCode & 0xff);
    //sendData.append(rscmd[511]);
    //my_serialport->readAll();
    qDebug()<<sendData.toHex(' ');
    my_serialport2->write(sendData);
    rs422_frame_num++;

    //qDebug() << "sendrs422Data:"<<sendData.toHex();

    //     QByteArray sendData;
    //     unsigned char rscmd[200] = {0x00};
    //     //RS422载荷数据
    //     rscmd[0] = 0xD3;
    //     rscmd[1] = 0x43;
    //     short n = 200;
    //     char p[2];
    //     memcpy(p, (char*)&n, sizeof(short));
    //     rscmd[2] = p[0];
    //     rscmd[3] = p[1];

    //     char num[2];
    //     memcpy(num, (char*)&rs422_frame_num, sizeof(unsigned short));
    //     rscmd[4] = num[0];
    //     rscmd[5] = num[1];

    //     for(int i= 0; i < 193;i++)
    //     {
    //         rscmd[6+i] = (unsigned char)i;
    //     }
    //     int iCheckCode = 0;
    //     for (int i = 0; i < 199; i++)
    //     {
    //         iCheckCode += rscmd[i];
    //         sendData.append(rscmd[i]);
    //     }
    //     rscmd[199] = (unsigned char)(iCheckCode & 0xff);
    //     sendData.append(rscmd[199]);
    //     my_serialport->readAll();
    //     my_serialport->write(sendData);
    //     rs422_frame_num++;

}
void MainWindow::SendComDta()
{
    if(m_spSt > 3 && !UseKey)
    {
        //表示串口断开
        ui->treeARM->clear();
        ui->treeCtrl->clear();
        if(m_spSt % 2 == 0)
            ui->label_10->setStyleSheet("background-color: rgb(0, 55, 55)");
        else
            ui->label_10->setStyleSheet("background-color: rgb(255, 0, 0)");
    }
    m_spSt++;
    return;

    if(++m_iContimes == 4)
    {
        m_bySendconstatus = 1;
    }

    if(!IsStopCom) {
        //        QByteArray sendData;

        //        unsigned char bytime[8]={0x00};
        //        unsigned char bycmd[16] = {0x00};
        //        bycmd[0] = 0xc7;
        //        bycmd[1] = 0xa7;
        //        bycmd[2] = 0x10;
        //        bycmd[4] = GetSysTime2Byte(bytime)[0];
        //        bycmd[3] = GetSysTime2Byte(bytime)[1];
        //        bycmd[5] = GetSysTime2Byte(bytime)[2];
        //        bycmd[6] = GetSysTime2Byte(bytime)[3];
        //        bycmd[7] = GetSysTime2Byte(bytime)[4];
        //        bycmd[8] = GetSysTime2Byte(bytime)[5];
        //        bycmd[9] = GetSysTime2Byte(bytime)[6];
        //        bycmd[10] = GetSysTime2Byte(bytime)[7];
        //        bycmd[11] = m_bySendBigCmd; ///大区控制指令

        //        /// 版本1,0
        //        bycmd[11] = m_bySendSmallCmd;   ///小区控制指令
        //        bycmd[12] = m_bySendconstatus;  /// 发送状态

        //        /// 版本2.0 时间：2018-12-28 修改内容：删除小区控制指令及指令状态指令
        //        ///

        //        /// 版本3.0 时间：2019-09-06 修改内容：参照滕盾20190906新协议
        //        //bycmd[14] = (unsigned char)m_iFramCnt++;
        //        if(m_bySendBigCmd == 0x41
        //                || m_bySendBigCmd == 0x42
        //                || m_bySendBigCmd == 0x43
        //                || m_bySendBigCmd == 0x44)
        //        {
        //            char rate;
        //            char gop;
        //            int rate_index = ui->comboBox->currentIndex();
        //            int gop_index = ui->comboBox_2->currentIndex();

        //            switch(rate_index)
        //            {
        //            case 0: rate = 0x1;break;
        //            case 1: rate = 0x2;break;
        //            case 2: rate = 0x4;break;
        //            case 3: rate = 0x8;break;
        //            default: rate = 0x0;break;
        //            }
        //            switch(gop_index)
        //            {
        //            case 0: gop = 0x1;break;
        //            case 1: gop = 0x2;break;
        //            case 2: gop = 0x4;break;
        //            case 3: gop = 0x8;break;
        //            default: gop = 0x0;break;
        //            }
        //            char val = (gop << 4)  |  rate;
        //            bycmd[12] = val;
        //        }
        //        else
        //        {
        //            bycmd[12] = 0x0;
        //        }
        //        int iCheckCode = 0;
        //        for (int i = 0; i < 15; i++)
        //        {
        //            iCheckCode += bycmd[i];
        //            sendData.append(bycmd[i]);
        //        }
        //        bycmd[15] = (unsigned char)(iCheckCode & 0xff);

        //        sendData.append(bycmd[15]);

        //        my_serialport->readAll();
        //        my_serialport->write(sendData);

        //        m_bySendBigCmd = 0;

        //qDebug() << sendData.toHex();

        if(m_spSt > 3)
        {
            //表示串口断开
            ui->treeARM->clear();
            ui->treeCtrl->clear();
            if(m_spSt % 2 == 0)
                ui->label_10->setStyleSheet("background-color: rgb(0, 55, 55)");
            else
                ui->label_10->setStyleSheet("background-color: rgb(255, 0, 0)");
        }
        frame_num++;

        if(frame_num == 10)
        {
            //1s 发送一帧遥感数据
            m_spSt++;
            //RS_sendData->start(100);
            if(m_sendRS422)
            {
                // RSSendData();
            }
            SendRemoteData();
            frame_num = 0;
        }
        else
        {
            if(m_sendRS422)
            {
                //            QDateTime current_date_time = QDateTime::currentDateTime();
                //            QString current_time = current_date_time.toString("hh:mm:ss.zzz ");
                //            qDebug() << "r422sendtime:"<<current_time;
                // RSSendData();
            }
        }
        if(m_iFramCnt == 256)
        {
            m_iFramCnt = 0;
        }
    }

}

void MainWindow::GetComDta()
{
    m_iContimes = 0;

    m_spSt = 0;//能接收数据
    ui->label_10->setStyleSheet("background-color: rgb(0, 255, 0)");

//    QString st("b5 a9 37 07 00 14 00 75 19 75 19 1d 09 fd "
//                "01 5f 00 00 00 00 00 00 02 00 02 02 00 00 "
//                "00 00 7f 7f 07 f0 01 00 00 26 27 00 dd 33 "
//                "00 00 00 00 03 00 00 00 00 00 00 00 00 00 "
//                "01 15 c1");
//    QByteArray b5;
//    QStringList sll = st.split(" ");
//    for(int i = 0; i < sll.size();i++)
//    {
//        b5[i] = sll[i].toInt(0, 16);
//    }

    QByteArray readAllData;
    readAllData = my_serialport->read(2);
//    readAllData = b5;
    if(readAllData.size() < 2)
        return;
    if((u_char)(readAllData.at(0)) == 0xAA && (u_char)(readAllData.at(1)) == 0x55)
    {
        readAllData.append(my_serialport->read(30));
        if(readAllData.length() != 32)
        {
            return ;
        }
        emit ParseData_AA55(readAllData);
    }
#ifdef PB_57
    else if((u_char)(readAllData.at(0)) == 0x99 && (u_char)(readAllData.at(1)) == 0x66)
    {
        readAllData.append(my_serialport->read(39));
        if(readAllData.length() != 41)
        {
            return;
        }
        emit ParseData_B5A9(readAllData);
    }
#endif

#ifdef PB_85
    else if((u_char)(readAllData.at(0)) == 0xB5 && (u_char)(readAllData.at(1)) == 0xA9)
    {
        readAllData.append(my_serialport->read(57));//版本不一样，读取的数据长度不一样 《调试信息协议 - 20200116》

        if(readAllData.length() != 59)
        {
            return;
        }

        int icook = 0;
        for(int i = 0 ;i < 58;i++)
        {
            icook += (unsigned char)readAllData[i];
        }

        qDebug()<<"jin"<<readAllData.toHex(' ')<<(unsigned char)(icook & 0xff);
        if((unsigned char)(icook & 0xff) == (unsigned char)readAllData[58])
        {
            if(UseKey)
            {
                QByteArray B5A9;
                B5A9.resize(59);
                QString st1("b5 a9 37 07 00 14 00 75 19 75 19 1d 09 fd "
                            "01 5f 00 00 00 00 00 00 02 00 02 02 00 00 "
                            "00 00 7f 7f 07 f0 01 00 00 26 27 00 dd 33 "
                            "00 00 00 00 03 00 00 00 00 00 00 00 00 00 "
                            "01 15 c1");
                QStringList sll = st1.split(" ");
                for(int i = 0; i < sll.size();i++)
                {
                    B5A9[i] = sll[i].toInt(0, 16);
                }
                B5A9[3] = readAllData[3];
                B5A9[4] = readAllData[4];
                B5A9[5] = readAllData[5];
                B5A9[6] = readAllData[6];
                B5A9[7] = readAllData[7];
                B5A9[8] = readAllData[8];
                B5A9[9] = readAllData[9];
                B5A9[10] = readAllData[10];
                B5A9[11] = readAllData[11];
                B5A9[12] = readAllData[12];
                B5A9[13] = readAllData[13];
                B5A9[14] = readAllData[14];
                B5A9[15] = readAllData[15];


                B5A9[45] = readAllData[45];
                B5A9[46] = readAllData[46];
                B5A9[47] = readAllData[47];
                B5A9[48] = readAllData[48];
                B5A9[54] = readAllData[54];
                B5A9[55] = readAllData[55];

                emit ParseData_B5A9(B5A9);
            }
            else
            {
emit ParseData_B5A9(readAllData);


            }
        }

    }
#endif

#ifdef PB_85
    else if((u_char)(readAllData.at(0)) == 0xc7 && (u_char)(readAllData.at(1)) == 0xa7 && notself)
    {
        readAllData.append(my_serialport->read(22));//版本不一样，读取的数据长度不一样 《调试信息协议 - 20200116》
        //        cout << "接收的数据: " << "size:" << readAllData.length() << "  ";
        //        QByteArray d = readAllData.toHex().toUpper();
        //        for (int i = 0; i < d.size(); i++)
        //        {
        //            if (i % 2 == 0)
        //                cout << d[i];
        //            else
        //                cout << d[i] << " ";
        //        }
        //        cout << endl;
        if(readAllData.length() != 24)
        {
            return;
        }
        else
        {
            //校验
            int icook = 0;
            for(int i = 0 ;i < 23;i++)
            {
                icook += (unsigned char)readAllData[i];
            }
            if((unsigned char)(icook & 0xff) == (unsigned char)readAllData[23])
            {
                if(UseKey)
                {
                    QByteArray c7A7;

                    c7A7.resize(24);

                    QString st("c7 a7 18 26 00 00 00 58 02 04 f3 0f f3 0f 02 02 00 00 a9 0f 01 00 00 cb");
                    QStringList sli = st.split(" ");
                    for(int i = 0; i < sli.size();i++)
                    {
                        c7A7[i] = sli[i].toInt(0, 16);
                    }

                    c7A7[3] = readAllData[3];
                    c7A7[7] = readAllData[7];
                    c7A7[8] = readAllData[8];
                    c7A7[9] = readAllData[9];
                    c7A7[10] = readAllData[10];
                    c7A7[11] = readAllData[11];
                    c7A7[12] = readAllData[12];
                    c7A7[13] = readAllData[13];
                    c7A7[14] = readAllData[14];
                    c7A7[15] = readAllData[15];
                    c7A7[16] = readAllData[16];
                    c7A7[17] = readAllData[17];
                    c7A7[20] = readAllData[20];
                    emit ParseData_C7A7(c7A7);

                }
                else
                {
                    emit ParseData_C7A7(readAllData);
                }

            }
            else
            {
                ui->statusBar->showMessage("user info crc err");
                return;
            }
        }

    }
#endif
    else if((u_char)(readAllData.at(0)) == 0xb5 && (u_char)(readAllData.at(1)) == 0xa9)
    {
        //readAllData.append(my_serialport->read(42));
        readAllData.append(my_serialport->read(45));
        //        cout << "接收的数据: " << "size:" << readAllData.length() << "  ";
        //        QByteArray d = readAllData.toHex().toUpper();
        //        for (int i = 0; i < d.size(); i++)
        //        {
        //            if (i % 2 == 0)
        //                cout << d[i];
        //            else
        //                cout << d[i] << " ";
        //        }
        //        cout << endl;

        //if(readAllData.length() != 44)
        if(readAllData.length() != 47)
        {
            return;
        }
        else
        {
            //校验
            int icook = 0;
            for(int i = 0 ;i < 46;i++)
            {
                icook += (unsigned char)readAllData[i];
            }
            if((unsigned char)(icook & 0xff) == (unsigned char)readAllData[46])
            {
                emit ParseData_B5A9(readAllData);
            }
            else
            {
                ui->statusBar->showMessage("debug info crc err");
                return;
            }
        }

    }
    else
    {
        //if(my_serialport->bytesAvailable() > 2048)
        my_serialport->readAll();

        //qDebug()<<c7a7data.toHex(' ');
        return;
    }

    return ;
}


void MainWindow::On_stop_cmd()//暂停发送命令
{
    if(IsStopCom)
    {
        //QMessageBox::information(this, tr("提示"), tr("串口未初始化,请先初始化串口"), QMessageBox::Ok);
        //刷新一下串口
        ui->cbox_coms->clear();
        ui->cbox_coms_2->clear();
        ui->cbox_coms_3->clear();
        GetAllCOMS();
        return ;
    }
    else
    {
        //ui->statusBar->showMessage(tr("停止%1串口通信").arg(ui->cbox_coms->currentText()),3000);
        return;
    }

    //    if(!IsStopCom) //运行中
    //    {
    //        IsStopCom = true;
    //        TM_sendData->stop();
    //        my_serialport->close();
    //        m_spSt = 0;
    //        ui->statusBar->showMessage(tr("停止%1串口通信").arg(ui->cbox_coms->currentText()),3000);
    //        //刷新一下串口
    //        ui->cbox_coms->clear();
    //        GetAllCOMS();
    //    }

}

void MainWindow::On_zhijian()//自检
{
    //0x11 – 自检
    m_bySendBigCmd = 0x11;
    SendRemoteData();
    ui->statusBar->showMessage(tr("自检"));
}

void MainWindow::StartData(int num)
{
    if(checkbitValue(startData,num))
    {
        qDebug()<<"sfsfffffffffffaaaaaaaaaa";
        return;
    }

    else
        startData ^= (1<<num);
    tempData[9] = startData;
    tempData[10] = startData;
    emit ParseData_C7A7(tempData);
    qDebug()<<"sfsfffffffffff";
}
void MainWindow::StopData(int num)
{
    if(checkbitValue(startData,num))
        return;
    else
        startData ^= (1<<num);
    tempData[9] = startData;
    tempData[10] = startData;
    emit ParseData_C7A7(tempData);
}

void MainWindow::InEndtime()
{
    notself = true;
    on_btn_DOOFF_clicked();
    //    m_bySendBigCmd= 0x55;
    //    SendRemoteData();
}

void MainWindow::handleShortcut_X()
{
    UseKey = false;
    //    ui->treeARM->clear();
    //    ui->treeCtrl->clear();


}

void MainWindow::onNewConnection()
{
    QTcpSocket *tcpSocket = tcpServer->nextPendingConnection();
    connect(tcpSocket, &QTcpSocket::disconnected, this, [=]
    {
        //让该线程中的套接字断开连接
        tcpSocket->disconnectFromHost();//断开连接
    });
    tcpthread = new TcpThread(tcpSocket);
    tcpthread->datathread(arry);
    connect(tcpthread,SIGNAL(info_tcp(bool)),this,SLOT(info_tcp_monitor(bool)));

}

void MainWindow::info_tcp_monitor(bool info)
{

    if(info)
    {
        qDebug()<<"jin";
        ui->pushButton_21->setEnabled(true);
        if(ui->radioButton->isChecked())
        {
            tcpthread->start();
        }
    }
    else
    {
        qDebug()<<"jin2";
        ui->pushButton_21->setEnabled(false);
    }


}

void MainWindow::handleShortcut_Z()
{
    UseKey = true;
}

//大区
void MainWindow::On_s_HDSDI()//开启HD-SDI通道1视频记录
{
    // StartData(0);
    //0x12 – 启动HD-SDI通道1视频记录
    m_bySendBigCmd = 0x12;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启HD-SDI通道1视频记录"));
    qDebug()<<"sfsf";
}
void MainWindow::On_s_HDSDI2()
{
    // StartData(1);
    m_bySendBigCmd = 0x15;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启HD-SDI通道2视频记录"));
}
void MainWindow::On_s_HDSDI3()
{
    //StartData(2);
    m_bySendBigCmd = 0x57;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启同步422通道3记录"));
}

void MainWindow::On_s_PAL()//开启PAL
{
    //StartData(3);
    // 	0x17 – 启动PAL模拟视频记录
    m_bySendBigCmd = 0x59;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启同步422通道4记录"));
}

void MainWindow::On_s_Link1()//开启camerlink通道1记录
{
    //StartData(4);
    m_bySendBigCmd = 0x1D;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启camerlink记录"));
}

void MainWindow::On_s_Link2()//unuse
{

    m_bySendBigCmd = 0x21;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启camerlink通道2记录"));
}

void MainWindow::On_s_422_1()//开启422同步通道1记录
{
    //StartData(5);
    m_bySendBigCmd = 0x21;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启同步422通道1记录"));
}

void MainWindow::On_s_422_2()
{  
    m_bySendBigCmd = 0x24;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启422同步通道2记录"));
}

void MainWindow::On_s_422_3()//unuse
{
    m_bySendBigCmd = 0x2E;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启422同步通道3记录"));
}
void MainWindow::On_s_Asy422_1()
{
    //StartData(6);
    m_bySendBigCmd = 0x28;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启异步422通道1记录"));
}
void MainWindow::On_s_Asy422_2()
{
    // StartData(7);
    m_bySendBigCmd = 0x2E;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启异步422通道2记录"));
}

void MainWindow::On_s_ALL()//开启所有通道记录
{
    for(int i= 0 ;i <8;i++)
    {
        // StartData(i);
    }
    m_bySendBigCmd = 0x3B;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启所有通道记录"));
}
void MainWindow::On_s_data1()//开启同步422通道1下传数据
{
    m_bySendBigCmd = 0x5B;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启同步422通道1下传数据"));
}           

void MainWindow::On_s_data2()//开启同步422通道2下传数据
{
    m_bySendBigCmd = 0x5D;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启同步422通道2下传数据"));
} 

void MainWindow::On_s_data3()//开启同步422通道3下传数据
{
    m_bySendBigCmd = 0x5F;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启同步422通道3下传数据"));
} 

void MainWindow::On_s_data4()//开启同步422通道4下传数据
{
    m_bySendBigCmd = 0x61;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启同步422通道4下传数据"));
} 
void MainWindow::On_s_net()//开启网络通道数据下载
{
    m_bySendBigCmd = 0x63;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启网络通道数据下载"));
} 


void MainWindow::On_e_data1()//关闭同步422通道1下传数据
{
    m_bySendBigCmd = 0x5C;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止同步422通道1下传数据"));
}           

void MainWindow::On_e_data2()//开启同步422通道2下传数据
{
    m_bySendBigCmd = 0x5E;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止同步422通道2下传数据"));
} 

void MainWindow::On_e_data3()//开启同步422通道3下传数据
{
    m_bySendBigCmd = 0x60;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止同步422通道3下传数据"));
} 

void MainWindow::On_e_data4()//开启同步422通道4下传数据
{
    m_bySendBigCmd = 0x62;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止同步422通道4下传数据"));
} 
void MainWindow::On_e_net()//开启网络通道数据下载
{
    m_bySendBigCmd = 0x64;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止网络通道数据下载"));
} 






void MainWindow::On_e_HDSDI()//停止HD-SDI视频记录
{
    // StopData(0);
    m_bySendBigCmd = 0x14;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止HD-SDI通道1视频记录"));
}
void MainWindow::On_e_HDSDI2()
{
    // StopData(1);
    m_bySendBigCmd = 0x16;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止HD-SDI通道2视频记录"));
}
void MainWindow::On_e_HDSDI3()
{
    // StopData(2);
    m_bySendBigCmd = 0x58;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止同步422通道3记录"));
}

void MainWindow::On_e_PAL()//停止PAL
{
    // StopData(3);
    m_bySendBigCmd = 0x5A;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止同步422通道4记录"));
}

void MainWindow::On_e_Link1()//停止camerlink通道1记录
{
    // StopData(4);
    m_bySendBigCmd = 0x1E;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止camerlink记录"));
}

void MainWindow::On_e_Link2()//unuse
{
    m_bySendBigCmd = 0x22;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止camerlink通道2记录"));
}

void MainWindow::On_e_422_1()//停止422同步通道1记录
{
    // StopData(5);
    m_bySendBigCmd = 0x22;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止同步422通道1记录"));
}

void MainWindow::On_e_422_2()
{
    m_bySendBigCmd = 0x2D;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止422同步通道2记录"));
}

void MainWindow::On_e_422_3()//unuse
{
    m_bySendBigCmd = 0x33;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止422同步通道3记录"));
}
void MainWindow::On_e_Asy422_1()
{
    // StopData(6);
    m_bySendBigCmd = 0x2D;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止异步422通道1记录"));
}

void MainWindow::On_e_Asy422_2()
{
    //StopData(7);
    m_bySendBigCmd = 0x33;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止异步422通道2记录"));
}

void MainWindow::On_e_ALL()//停止所有通道记录
{
    for(int i = 0; i < 8 ;i++)
    {
        //StopData(i);
    }
    m_bySendBigCmd = 0x3C;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止所有通道记录"));
}
void MainWindow::On_DataDel()//数据销毁
{
    m_bySendBigCmd = 0xFE;
    SendRemoteData();
    ui->statusBar->showMessage(tr("数据销毁"));
}
void MainWindow::On_Sdi1Set()
{
    m_bySendBigCmd= 0x41;
    SendRemoteData();
    ui->statusBar->showMessage(tr("sdi-1编码参数设置"));
}
void MainWindow::On_sdi2Set()
{
    m_bySendBigCmd= 0x42;
    SendRemoteData();
    ui->statusBar->showMessage(tr("sdi-2编码参数设置"));
}
void MainWindow::On_sdi3Set()
{
    m_bySendBigCmd= 0x43;
    SendRemoteData();
    ui->statusBar->showMessage(tr("sdi-3编码参数设置"));
}
void MainWindow::On_sdi4Set()
{
    m_bySendBigCmd= 0x44;
    SendRemoteData();
    ui->statusBar->showMessage(tr("sdi-4编码参数设置"));
}
void MainWindow::On_SendRS422()
{
    if(m_sendRS422)
    {
        m_sendRS422 = false;
        ui->pushButton_10->setText(tr("发送RS422"));
        if(RS_sendData->isActive())
            RS_sendData->stop();
    }
    else
    {
        m_sendRS422 = true;
        ui->pushButton_10->setText(tr("停止发送RS422"));
        RS_sendData->start(500);
    }
    rs422_frame_num = 0;
}
void MainWindow::On_Restoration()
{

    m_bySendBigCmd= 0xFD;
    SendRemoteData();
    ui->statusBar->showMessage(tr("复位"));
}

//小区
void MainWindow::On_s_HDSDI_p()//开启HD-SDI视频记录
{
    m_bySendSmallCmd = 0xA3;
    ui->statusBar->showMessage(tr("开启小区HD-SDI视频记录"));
}

void MainWindow::On_s_PAL_p()//开启PAL
{
    m_bySendSmallCmd = 0xA6;
    ui->statusBar->showMessage(tr("开启小区PAL记录"));
}

void MainWindow::On_s_Link1_p()//开启camerlink通道1记录
{
    m_bySendSmallCmd = 0xAA;
    ui->statusBar->showMessage(tr("开启小区camerlink通道1记录"));
}

void MainWindow::On_s_Link2_p()
{
    m_bySendSmallCmd = 0xB1;
    ui->statusBar->showMessage(tr("开启小区camerlink通道2记录"));
}

void MainWindow::On_s_422_1_p()//开启422同步通道1记录
{
    m_bySendSmallCmd = 0xB4;
    ui->statusBar->showMessage(tr("开启小区422同步通道1记录"));
}

void MainWindow::On_s_422_2_p()
{
    m_bySendSmallCmd = 0xB8;
    ui->statusBar->showMessage(tr("开启小区422同步通道2记录"));
}

void MainWindow::On_s_422_3_p()
{
    m_bySendSmallCmd = 0xBE;
    ui->statusBar->showMessage(tr("开启小区422同步通道3记录"));
}

void MainWindow::On_s_ALL_p()//开启所有通道记录
{
    m_bySendSmallCmd = 0xC5;
    ui->statusBar->showMessage(tr("开启小区所有通道记录"));
}



void MainWindow::On_e_HDSDI_p()//停止HD-SDI视频记录
{
    m_bySendSmallCmd = 0xA5;
    ui->statusBar->showMessage(tr("停止小区HD-SDI视频记录"));
}

void MainWindow::On_e_PAL_p()//停止PAL
{
    m_bySendSmallCmd = 0xA9;
    ui->statusBar->showMessage(tr("停止小区PAL"));
}

void MainWindow::On_e_Link1_p()//停止小区camerlink通道1记录
{
    m_bySendSmallCmd = 0xAC;
    ui->statusBar->showMessage(tr("停止小区camerlink通道1记录"));
}

void MainWindow::On_e_Link2_p()
{
    m_bySendSmallCmd = 0xB2;
    ui->statusBar->showMessage(tr("停止小区camerlink通道2记录"));
}

void MainWindow::On_e_422_1_p()//停止422同步通道1记录
{
    m_bySendSmallCmd = 0xB7;
    ui->statusBar->showMessage(tr("停止小区422同步通道1记录"));
}

void MainWindow::On_e_422_2_p()
{
    m_bySendSmallCmd = 0xBD;
    ui->statusBar->showMessage(tr("停止小区422同步通道2记录"));
}

void MainWindow::On_e_422_3_p()
{
    m_bySendSmallCmd = 0xC3;
    ui->statusBar->showMessage(tr("停止小区422同步通道3记录"));
}

void MainWindow::On_e_ALL_p()//停止所有通道记录
{
    m_bySendSmallCmd = 0xC6;
    ui->statusBar->showMessage(tr("停止小区所有通道记录"));
}


void MainWindow::On_Tree_expand()
{
    //展开/合并Tree
    if(!IsTreeExpand)
    {
        IsTreeExpand = true;
        ui->btn_tree->setText("全部合并");
    }
    else
    {
        IsTreeExpand = false;
        ui->btn_tree->setText("全部展开");
    }
}


unsigned char * MainWindow::GetSysTime2Byte(unsigned char bytime[8])
{
    if(m_bIsMyTime)
    {
        bytime[0] = ui->le_year->text().toUShort() >> 8;
        bytime[1] = ui->le_year->text().toUShort() & 0xffff;
        bytime[2] = ui->le_yue->text().toUShort();
        bytime[3] = ui->le_ri->text().toUShort();
        bytime[4] = ui->le_shi->text().toUShort();
        bytime[5] = ui->le_fen->text().toUShort();
        bytime[6] = ui->le_miao->text().toUShort();
        bytime[7] = ui->le_haomiao->text().toUShort();

        return bytime;
    }

    SYSTEMTIME systime;
    GetLocalTime(&systime);

    bytime[0] = systime.wYear >> 8;
    bytime[1] = systime.wYear & 0xffff;
    bytime[2] = systime.wMonth;
    bytime[3] = systime.wDay;
    bytime[4] = systime.wHour;
    bytime[5] = systime.wMinute;
    bytime[6] = systime.wSecond;
    bytime[7] = systime.wMilliseconds;

    return bytime;
}

void MainWindow::InitParaName()
{
    /// 2018-12-18前版本
    /*
    g_strParaName[0] = "帧同步码";
    g_strParaName[1] = "帧同步码";
    g_strParaName[2] = "工作模式";
    g_strParaName[3] = "故障状态";
    g_strParaName[4] = "存储模块存在标志";
    g_strParaName[5] = "记录状态字1";
    g_strParaName[6] = "记录状态字2";
    g_strParaName[7] = "故障状态字1";
    g_strParaName[8] = "故障状态字2";
    g_strParaName[9] = "剩余容量(大区)";
    g_strParaName[10] = "大区最老数据时间-年";
    g_strParaName[11] = "大区最老数据时间-月";
    g_strParaName[12] = "大区最老数据时间-日";
    g_strParaName[13] = "大区最老数据时间-时";
    g_strParaName[14] = "大区最老数据时间-分";
    g_strParaName[15] = "大区最老数据时间-秒";
    g_strParaName[16] = "小区记录状态1";
    g_strParaName[17] = "小区记录状态2";
    g_strParaName[18] = "MDR剩余容量(小区)";
    g_strParaName[19] = "小区最老数据时间-年";
    g_strParaName[20] = "小区最老数据时间-月";
    g_strParaName[21] = "小区最老数据时间-日";
    g_strParaName[22] = "小区最老数据时间-时";
    g_strParaName[23] = "小区最老数据时间-分";
    g_strParaName[24] = "小区最老数据时间-秒";
    g_strParaName[25] = "主版本号";
    g_strParaName[26] = "次版本号";
    g_strParaName[27] = "备份";
    g_strParaName[28] = "帧计数";
    g_strParaName[29] = "验证码";
    */
    /*
    g_strParaName[0] = "帧同步码";
    g_strParaName[1] = "帧同步码";
    g_strParaName[2] = "工作模式";
    g_strParaName[3] = "故障状态";
    g_strParaName[4] = "硬盘存在状态";
    g_strParaName[5] = "记录状态字";
    g_strParaName[6] = "数据输入状态";
    g_strParaName[7] = "剩余容量(大区)";
    g_strParaName[8] = "大区最老数据时间-年";
    g_strParaName[9] = "大区最老数据时间-月";
    g_strParaName[10] = "大区最老数据时间-日";
    g_strParaName[11] = "大区最老数据时间-时";
    g_strParaName[12] = "大区最老数据时间-分";
    g_strParaName[13] = "大区最老数据时间-秒";
    g_strParaName[14] = "硬盘擦写次数";
    g_strParaName[15] = "通信字状态";
    g_strParaName[16] = "小区记录状态";
    g_strParaName[17] = "MDR剩余容量(小区)";
    g_strParaName[18] = "小区最老数据时间-年";
    g_strParaName[19] = "小区最老数据时间-月";
    g_strParaName[20] = "小区最老数据时间-日";
    g_strParaName[21] = "小区最老数据时间-时";
    g_strParaName[22] = "小区最老数据时间-分";
    g_strParaName[23] = "小区最老数据时间-秒";
    g_strParaName[24] = "主版本号";
    g_strParaName[25] = "次版本号";
    g_strParaName[26] = "备份";
    g_strParaName[27] = "帧计数";
    g_strParaName[28] = "验证码";
*/
    g_strParaName[0] = "帧同步码";
    g_strParaName[1] = "帧同步码";
    g_strParaName[2] = "工作模式";
    g_strParaName[3] = "故障状态";
    g_strParaName[4] = "硬盘总容量（低字节）";
    g_strParaName[5] = "硬盘总容量（高字节）";
    g_strParaName[6] = "记录状态字1";
    g_strParaName[7] = "记录状态字2";
    g_strParaName[8] = "剩余容量（%）";
    g_strParaName[9] = "通信状态字";
    //    g_strParaName[10] = "备份";
    //    g_strParaName[11] = "备份";
    //    g_strParaName[12] = "备份";
    //    g_strParaName[13] = "备份";
    //    g_strParaName[14] = "备份";
    //    g_strParaName[15] = "备份";
    //    g_strParaName[16] = "备份";
    //    g_strParaName[17] = "备份";
    //    g_strParaName[18] = "备份";
    //    g_strParaName[19] = "备份";
    //    g_strParaName[20] = "备份";
    //    g_strParaName[21] = "备份";
    //    g_strParaName[22] = "备份";
    //    g_strParaName[23] = "备份";
    //    g_strParaName[24] = "备份";
    //    g_strParaName[25] = "备份";
    //    g_strParaName[26] = "备份";
    g_strParaName[10] = "主版本号";
    g_strParaName[11] = "次版本号";
    //    g_strParaName[29] = "备份";
    g_strParaName[12] = "帧计数";
    g_strParaName[13] = "校验码";
#ifdef PB_57
    //Arm cmd para name
    g_strArmParaNmae[0] = "帧同步码";
    g_strArmParaNmae[1] = "帧同步码";
    g_strArmParaNmae[2] = "数管板状态1";
    g_strArmParaNmae[3] = "数管板状态2";
    g_strArmParaNmae[4] = "压缩板状态1";
    g_strArmParaNmae[5] = "压缩板状态2";
    g_strArmParaNmae[6] = "压缩板状态3";
    g_strArmParaNmae[7] = "压缩通道状态1";
    g_strArmParaNmae[8] = "压缩通道状态2";
    g_strArmParaNmae[9] = "压缩通道状态3";
    g_strArmParaNmae[10] = "压缩通道状态4";
    g_strArmParaNmae[11] = "压缩通道状态5";
    g_strArmParaNmae[12] = "压缩通道状态6";
    g_strArmParaNmae[13] = "压缩通道状态7";
    g_strArmParaNmae[14] = "存储板状态1";
    g_strArmParaNmae[15] = "存储板状态2";
    g_strArmParaNmae[16] = "存储板状态3";
    g_strArmParaNmae[17] = "存储板状态4";
    g_strArmParaNmae[18] = "数管板温度第一字节";
    g_strArmParaNmae[19] = "数管板温度第二字节";
    g_strArmParaNmae[20] = "数管板温度第三字节";
    g_strArmParaNmae[21] = "数管板温度第四字节";
    g_strArmParaNmae[22] = "压缩板温度第一字节";
    g_strArmParaNmae[23] = "压缩板温度第二字节";
    g_strArmParaNmae[24] = "压缩板温度第三字节";
    g_strArmParaNmae[25] = "压缩板温度第四字节";
    g_strArmParaNmae[26] = "存储盘工作状态1";
    g_strArmParaNmae[27] = "存储盘工作状态2";
    g_strArmParaNmae[28] = "存储盘工作状态3";
    g_strArmParaNmae[29] = "存储盘工作状态4";
    g_strArmParaNmae[30] = "盘1记录的之前的盘选择状态";
    g_strArmParaNmae[31] = "盘2记录的之前的盘选择状态";
    g_strArmParaNmae[32] = "盘3记录的之前的盘选择状态";
    g_strArmParaNmae[33] = "盘4记录的之前的盘选择状态";
    g_strArmParaNmae[34] = "盘5记录的之前的盘选择状态";
    g_strArmParaNmae[35] = "盘6记录的之前的盘选择状态";
    g_strArmParaNmae[36] = "盘7记录的之前的盘选择状态";
    g_strArmParaNmae[37] = "盘8记录的之前的盘选择状态";
    g_strArmParaNmae[38] = "下载状态";
    g_strArmParaNmae[39] = "帧计数";
    g_strArmParaNmae[40] = "校验码";

#endif

#ifdef PB_60
    //Arm cmd para name
    g_strArmParaNmae[0] = "帧同步码_B5";
    g_strArmParaNmae[1] = "帧同步码_A9";
    g_strArmParaNmae[2] = "压缩版状态1";
    g_strArmParaNmae[3] = "压缩版状态2";
    g_strArmParaNmae[4] = "压缩版状态3";
    g_strArmParaNmae[5] = "压缩版状态4";
    g_strArmParaNmae[6] = "压缩版温度低字节";
    g_strArmParaNmae[7] = "压缩版温度高字节";
    g_strArmParaNmae[8] = "CAM1通道状态1";
    g_strArmParaNmae[9] = "CAM1通道状态2";
    g_strArmParaNmae[10] = "CAM1通道状态3";
    g_strArmParaNmae[11] = "CAM1通道状态4";
    g_strArmParaNmae[12] = "CAM2通道状态1";
    g_strArmParaNmae[13] = "CAM2通道状态2";
    g_strArmParaNmae[14] = "CAM2通道状态3";
    g_strArmParaNmae[15] = "CAM2通道状态4";
    g_strArmParaNmae[16] = "压缩通道状态1";
    g_strArmParaNmae[17] = "压缩通道状态2";
    g_strArmParaNmae[18] = "CAM3通道状态1";
    g_strArmParaNmae[19] = "CAM3通道状态2";
    g_strArmParaNmae[20] = "写盘状态";
    g_strArmParaNmae[21] = "大区盘是否存在";
    g_strArmParaNmae[22] = "大区剩余容量";
    g_strArmParaNmae[23] = "计数";

#endif
}


void MainWindow::On_connect()
{
    // 0x02 - 请求连接
    SendARMcmd(0,0x02);
    ui->statusBar->showMessage("ARM 请求连接");
}

void MainWindow::On_FileCmd()
{
    //0x03 - 文件列表请求
    SendARMcmd(0,0x03);
    ui->statusBar->showMessage("ARM 文件列表请求");
}

void MainWindow::On_qDownLoad()
{
    //0x04 - 快速下载命令
    SendARMcmd(0,0x04);
    ui->statusBar->showMessage("ARM 快速下载");
}

void MainWindow::On_ResetUnit()
{
    //0x05 - 复位存储单元
    SendARMcmd(0,0x05);
    ui->statusBar->showMessage("ARM 复位存储单元");
}

void MainWindow::On_FormatStorage()
{
    //0x06 - 格式化存储
    SendARMcmd(0,0x06);
    ui->statusBar->showMessage("ARM 格式化存储");
}

void MainWindow::On_WriteRegister()
{
    /*   QByteArray readAllData;
    QString str("B5 A9 2F 13 00 35 19 75 1C 13 00 75 1C 75 1C 18 07 0D 0B 05 02 14 03 00 00 00 00 3A DD 5D 0D 33 00 00 00 00 55 0E 86 06 00 03 00 00 B5 01 AF");
    QStringList sli = str.split(" ");

    for (size_t i = 0; i < sli.size(); i++)
    {
        readAllData.append(sli[i].toInt(0, 16));
    }
    emit ParseData_B5A9(readAllData);
    ui->statusBar->showMessage(tr("ARM 打开调试信息"));
    //return;
*/
    m_bySendBigCmd = 0xdb;
    SendRemoteData();
    ui->statusBar->showMessage(tr("ARM 打开调试信息"));

    //0x09 打开调试信息
    //    SendARMcmd(0,0x09);
    //    ui->statusBar->showMessage("ARM 打开调试信息");
}

void MainWindow::On_ReadRegister()
{
    ui->treeARM->clear();
    ui->statusBar->showMessage(tr("ARM 关闭调试信息"));
    //return;

    m_bySendBigCmd = 0xdc;
    SendRemoteData();
    ui->statusBar->showMessage(tr("ARM 关闭调试信息"));
    ui->treeARM->clear();
    CloseStatus = true;
    if(closeARM == NULL)
    {
        closeARM = new QTimer(this);
        connect(this->closeARM,SIGNAL(timeout()),this,SLOT(CloseARMStatus()));
    }
    closeARM->setSingleShot(true);
    closeARM->start(1000);



    //0x0A 关闭调试信息
    //    SendARMcmd(0,0x0A);
    //    ui->statusBar->showMessage("ARM 关闭调试信息");
    //    ui->treeARM->clear();
}
void MainWindow::On_PcieReset()
{
    m_bySendBigCmd = 0xde;
    SendRemoteData();
    ui->statusBar->showMessage(tr("PCIE复位"));
}


void MainWindow::On_WriteSpeedTest()
{
    m_bySendBigCmd = 0xda;
    SendRemoteData();
    ui->statusBar->showMessage(tr("写入速度测试"));
}


void MainWindow::Tree_AddItem(QTreeWidgetItem *_parent,QString _name,int &_color)
{
    QTreeWidgetItem *_items = new QTreeWidgetItem(QStringList(_name));

    if(_color == 1)
    {
        _items->setTextColor(0,Qt::red);
    }
    else if(_color == 2)
    {
        _items->setTextColor(0,Qt::blue);
    }
    else if(_color == 3)
    {
        _items->setTextColor(0,Qt::green);
    }
    else if(_color == 0)
    {
        _items->setTextColor(0,Qt::black);
    }
    else if(_color == 4)
    {
        _items->setTextColor(0,Qt::darkGreen);
    }
    _parent->addChild(_items);
    _color = 0;
}

void MainWindow::To_ParseData_AA55(QByteArray readAllData)
{
    /// 解析记录器回传给计算机的指令 V2.0 ADD:2018-12-28
    /// 解析记录器回传给计算机的指令 V3.0 ADD:2019-09-06
    /* ui->treeCtrl->clear();

    QTreeWidgetItem *item_root = new QTreeWidgetItem(ui->treeCtrl);

    item_root->setText(0,"用户结果集解析");
    int ifcolor=0;//颜色控制
    QString strTemp;//
    int iTemp = 0;//解析的第几字节
    int iRoot = 0;//树结构子节点计数
    u_int tot;//硬盘总容量

    //0 0xAA  帧同步码
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "帧同步码";
    Tree_AddItem(item_root,strTemp,ifcolor);
    iRoot++;

    //1 0x55  帧同步码
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "帧同步码";
    Tree_AddItem(item_root,strTemp,ifcolor);
    iRoot++;

    //2 工作模式
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "工作模式";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if((u_char)readAllData[iTemp] == 0x23){
        strTemp = "MBIT";
    }else if((u_char)readAllData[iTemp] == 0x25){
        strTemp = "IBIT";
    }else if((u_char)readAllData[iTemp] == 0x26){
        strTemp = "NORMAL";
        ifcolor = 0;
    }else{
        strTemp = "无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    iRoot++;

    //3 故障状态
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "故障状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)readAllData[iTemp],0))
    {
        strTemp = "硬盘故障状态：故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘故障状态：正常";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],1))
    {
        strTemp = "数据记录故障状态：故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据记录故障状态：正常";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],2))
//    {
//        strTemp = "HD-SDI记录故障状态：故障";
//        ifcolor = 1;
//    }
//    else
//    {
//        strTemp = "HD-SDI记录故障状态：正常";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],3))
//    {
//        strTemp = "Camera Link通道1记录故障状态：故障";
//        ifcolor = 1;
//    }
//    else
//    {
//        strTemp = "Camera Link通道1记录故障状态：正常";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],4))
//    {
//        strTemp = "Camera Link通道2记录故障状态：故障";
//        ifcolor = 1;
//    }
//    else
//    {
//        strTemp = "Camera Link通道2记录故障状态：正常";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],5))
//    {
//        strTemp = "同步422通道1记录故障状态：故障";
//        ifcolor = 1;
//    }
//    else
//    {
//        strTemp = "同步422通道1记录故障状态：正常";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],6))
//    {
//        strTemp = "同步422通道2记录故障状态：故障";
//        ifcolor = 1;
//    }
//    else
//    {
//        strTemp = "同步422通道2记录故障状态：正常";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],7))
//    {
//        strTemp = "同步422通道3记录故障状态：故障";
//        ifcolor = 1;
//    }
//    else
//    {
//        strTemp = "同步422通道3记录故障状态：正常";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

    iRoot++;

    //4~5 硬盘总容量
    iTemp++;
    tot = (u_char)readAllData.at(iTemp);
    iTemp++;
    tot |= ((u_char)readAllData.at(iTemp) << 8);
    strTemp.sprintf("0x%04x:",tot);
    strTemp += "硬盘总容量";
    Tree_AddItem(item_root,strTemp,ifcolor);


    strTemp.sprintf("%.2fT", tot/100.0);
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    iRoot++;

    //6 记录状态字1
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "记录状态字1";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(((u_char)readAllData[iTemp]&0b11) == 0)
    {
        strTemp = "PAL模拟视频记录状态：未记录";
    }
    else if(((u_char)readAllData[iTemp]&0b11) == 1)
    {
        strTemp = "PAL模拟视频记录状态：正常记录";
        ifcolor = 2;
    }
    else
    {
        strTemp = "PAL模拟视频记录状态：记录故障";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]>>2)&0b11) == 0)
    {
        strTemp = "HD-SDI数字视频记录状态：未记录";
    }
    else if((((u_char)readAllData[iTemp]>>2)&0b11) == 1)
    {
        strTemp = "HD-SDI数字视频记录状态：正常记录";
        ifcolor = 2;
    }
    else
    {
        strTemp = "HD-SDII数字视频记录状态：记录故障";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]>>4)&0b11) == 0)
    {
        strTemp = "Camera Link通道1记录状态：未记录";
    }
    else if((((u_char)readAllData[iTemp]>>4)&0b11) == 1)
    {
        strTemp = "Camera Link通道1记录状态：正常记录";
        ifcolor = 2;
    }
    else
    {
        strTemp = "Camera Link通道1记录状态：记录故障";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]>>6)&0b11) == 0)
    {
        strTemp = "Camera Link通道2记录状态：未记录";
    }
    else if((((u_char)readAllData[iTemp]>>6)&0b11) == 1)
    {
        strTemp = "Camera Link通道2记录状态：正常记录";
        ifcolor = 2;
    }
    else
    {
        strTemp = "Camera Link通道2记录状态：停止记录";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],4))
//    {
//        strTemp = "同步422通道1记录状态：记录";
//        ifcolor = 2;
//    }
//    else
//    {
//        strTemp = "同步422通道1记录状态：停止记录";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],5))
//    {
//        strTemp = "同步422通道2记录状态：记录";
//        ifcolor = 2;
//    }
//    else
//    {
//        strTemp = "同步422通道2记录状态：停止记录";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],6))
//    {
//        strTemp = "同步422通道3记录状态：记录";
//        ifcolor = 2;
//    }
//    else
//    {
//        strTemp = "同步422通道3记录状态：停止记录";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

    iRoot++;

    //7 记录状态字2
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "记录状态字2";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(((u_char)readAllData[iTemp]&0b11) == 0)
    {
        strTemp = "同步422通道1记录状态：未记录";
    }
    else if(((u_char)readAllData[iTemp]&0b11) == 1)
    {
        strTemp = "同步422通道1记录状态：正常记录";
        ifcolor = 2;
    }
    else
    {
        strTemp = "同步422通道1记录状态：记录故障";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]>>2)&0b11) == 0)
    {
        strTemp = "同步422通道2记录状态：未记录";
    }
    else if((((u_char)readAllData[iTemp]>>2)&0b11) == 1)
    {
        strTemp = "同步422通道2记录状态：正常记录";
        ifcolor = 2;
    }
    else
    {
        strTemp = "同步422通道2记录状态：记录故障";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]>>4)&0b11) == 0)
    {
        strTemp = "同步422通道3记录状态：未记录";
    }
    else if((((u_char)readAllData[iTemp]>>4)&0b11) == 1)
    {
        strTemp = "同步422通道3记录状态：正常记录";
        ifcolor = 2;
    }
    else
    {
        strTemp = "同步422通道3记录状态：记录故障";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;
    iRoot++;

    //6 数据输入状态
//    iTemp++;
//    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
//    strTemp += "数据输入状态";
//    Tree_AddItem(item_root,strTemp,ifcolor);

//    if(checkbitValue((u_char)readAllData[iTemp],0))
//    {
//        strTemp = "PAL模拟视频输入状态：有输入";
//        ifcolor = 2;
//    }
//    else
//    {
//        strTemp = "PAL模拟视频输入状态：无输入";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],1))
//    {
//        strTemp = "HD-SDI数字视频输入状态：有输入";
//        ifcolor = 2;
//    }
//    else
//    {
//        strTemp = "HD-SDI数字视频输入状态：无输入";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],2))
//    {
//        strTemp = "Camera Link通道1输入状态：有输入";
//        ifcolor = 2;
//    }
//    else
//    {
//        strTemp = "Camera Link通道1输入状态：无输入";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],3))
//    {
//        strTemp = "Camera Link通道2输入状态：有输入";
//        ifcolor = 2;
//    }
//    else
//    {
//        strTemp = "Camera Link通道2输入状态：无输入";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],4))
//    {
//        strTemp = "同步422通道1输入状态：有输入";
//        ifcolor = 2;
//    }
//    else
//    {
//        strTemp = "同步422通道1输入状态：无输入";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],5))
//    {
//        strTemp = "同步422通道2输入状态：有输入";
//        ifcolor = 2;
//    }
//    else
//    {
//        strTemp = "同步422通道2输入状态：无输入";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    if(checkbitValue((u_char)readAllData[iTemp],6))
//    {
//        strTemp = "同步422通道3输入状态：有输入";
//        ifcolor = 2;
//    }
//    else
//    {
//        strTemp = "同步422通道3输入状态：无输入";
//    }
//    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
//    ifcolor=0;

//    iRoot++;

    //8 剩余容量
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "剩余容量";
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp.sprintf("%d%%",(u_char)readAllData.at(iTemp));
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    iRoot++;

    //9 通信状态字
    iTemp = 17;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "通信状态字";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)readAllData[iTemp],0))
    {
        strTemp = "通信状态：故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "通信状态：正常";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    iRoot++;

    //10 ~ 26 备份

    //27 主版本号
    iTemp = 27;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "主版本号";
    Tree_AddItem(item_root,strTemp,ifcolor);
    iRoot++;

    //28 次版本号
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "次版本号";
    Tree_AddItem(item_root,strTemp,ifcolor);
    iRoot++;

    //29 备份
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "备份";
    Tree_AddItem(item_root,strTemp,ifcolor);
    iRoot++;

    //30 帧计数
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "帧计数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    iRoot++;

    //31 校验码
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += "校验码";
    Tree_AddItem(item_root,strTemp,ifcolor);
    iRoot++;

    if(IsTreeExpand)
        ui->treeCtrl->expandAll();
    return ;
    */
}

void MainWindow::To_ParseData_AA55_20181228(QByteArray readAllData)
{
    /// 解析记录器回传给计算机的指令 V1.0 :2018-12-28
    ui->treeCtrl->clear();

    QTreeWidgetItem *item_root = new QTreeWidgetItem(ui->treeCtrl);//传递父对象

    item_root->setText(0,"用户结果集解析");

    int ifcolor=0;
    QString strTemp;
    int iTemp = 0;
    //0 0xaa  帧同步码  itemp=0,root=0
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);


    //1 0x55  帧同步码 itemp=1,root=1
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //2 0x26 工作模式  itemp=2,root=2
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if((u_char)readAllData[iTemp] == 0x23){
        strTemp = "MBIT";
    }else if((u_char)readAllData[iTemp] == 0x25){
        strTemp = "IBIT";
    }else if((u_char)readAllData[iTemp] == 0x26){
        strTemp = "NORMAL";
        ifcolor = 0;
    }else{
        strTemp = "无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(2),strTemp,ifcolor);
    ifcolor=0;

    //3 0x29 故障状态 itemp=3,root=3
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if((u_char)readAllData[iTemp] == 0x29){
        strTemp = "正常";
    }else if((u_char)readAllData[iTemp] == 0x2A){
        strTemp = "故障";
        ifcolor = 1;
    }else{
        strTemp = "无效";
        ifcolor = 2;
    }

    Tree_AddItem(item_root->child(3),strTemp,ifcolor);
    ifcolor=0;


    //4 0x32 存储模块存在标志 itemp=4,root=4  /// 2018-12-18之前版本
    //    iTemp++;
    //    strTemp.sprintf("0x%02x:",(u_char)readAllData[iTemp]);
    //    strTemp += g_strParaName[iTemp];
    //    Tree_AddItem(item_root,strTemp,ifcolor);
    //    ifcolor=0;

    //    if((u_char)readAllData[iTemp] == 0x31){
    //        strTemp = "存储模块存在";
    //    }else if((u_char)readAllData[iTemp] == 0x32){
    //        strTemp = "存储模块不存在";
    //        ifcolor = 1;
    //    }else{
    //        strTemp = "无效";
    //        ifcolor = 2;
    //    }

    //    Tree_AddItem(item_root->child(4),strTemp,ifcolor);
    //    ifcolor=0;

    /// 4bit 硬盘存在状态 itemp=4,root=4
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData[iTemp]);
    strTemp += g_strParaName[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],0))
    {
        strTemp = "硬盘1: 不存在";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘1: 存在";
    }
    Tree_AddItem(item_root->child(4),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],1))
    {
        strTemp = "硬盘2: 不存在";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘2: 存在";
    }
    Tree_AddItem(item_root->child(4),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],2))
    {
        strTemp = "硬盘3: 不存在";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘3: 存在";
    }
    Tree_AddItem(item_root->child(4),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],3))
    {
        strTemp = "硬盘4: 不存在";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘4: 存在";
    }
    Tree_AddItem(item_root->child(4),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],4))
    {
        strTemp = "硬盘5: 不存在";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘5: 存在";
    }
    Tree_AddItem(item_root->child(4),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],5))
    {
        strTemp = "硬盘6: 不存在";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘6: 存在";
    }
    Tree_AddItem(item_root->child(4),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],6))
    {
        strTemp = "硬盘7: 不存在";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘7: 存在";
    }
    Tree_AddItem(item_root->child(4),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],7))
    {
        strTemp = "硬盘8: 不存在";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘8: 存在";
    }
    Tree_AddItem(item_root->child(4),strTemp,ifcolor);
    ifcolor=0;

    //5 0xff 记录状态字1 itemp=5,root=5  2018-12-18前版本
    /*
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x03)) == 0x01){
        strTemp = "PAL模拟视频记录状态:停止记录";
    }else if((((u_char)readAllData[iTemp]) & (0x03)) == 0x03){
        strTemp = "PAL模拟视频记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "PAL模拟视频记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(5),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x0C)) == 0x04){
        strTemp = "HD-SDI数字视频记录状态:停止记录";
    }else if((((u_char)readAllData[iTemp]) & (0x0C)) == 0x0C){
        strTemp = "HD-SDI数字视频记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "HD-SDI数字视频记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(5),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x30)) == 0x10){
        strTemp = "Camera Link通道1记录状态:停止记录";
        //                ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0x30)) == 0x30){
        strTemp = "Camera Link通道1记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "Camera Link通道1记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(5),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0xC0)) == 0x40){
        strTemp = "Camera Link通道2记录状态:停止记录";
        //                ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0xC0)) == 0xC0){
        strTemp = "Camera Link通道2记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "Camera Link通道2记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(5),strTemp,ifcolor);
    ifcolor=0;
    */

    //5 记录状态字 itemp=5,root=5
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],0))
    {
        strTemp = "PAL模拟视频记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "PAL模拟视频记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(5),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],1))
    {
        strTemp = "HD-SDI数字视频记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "HD-SDI数字视频记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(5),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],2))
    {
        strTemp = "Camera Link通道1记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "Camera Link通道1记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(5),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],3))
    {
        strTemp = "Camera Link通道2记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "Camera Link通道2记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(5),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],4))
    {
        strTemp = "同步422通道1记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道1记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(5),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],5))
    {
        strTemp = "同步422通道2记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道2记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(5),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],6))
    {
        strTemp = "同步422通道3记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道3记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(5),strTemp,ifcolor);
    ifcolor=0;

    //6 0xff 记录状态字2 itemp=6,root=6 2018-12-18前版本
    /*
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x03)) == 0x01){
        strTemp = "同步422通道1记录状态:停止记录";
    }else if((((u_char)readAllData[iTemp]) & (0x03)) == 0x03){
        strTemp = "同步422通道1记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "同步422通道1记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(6),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x0C)) == 0x04){
        strTemp = "同步422通道2记录状态:停止记录";
        //                ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0x0C)) == 0x0C){
        strTemp = "同步422通道2记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "同步422通道2记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(6),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x30)) == 0x10){
        strTemp = "同步422通道3记录状态:停止记录";
        //                ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0x30)) == 0x30){
        strTemp = "同步422通道3记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "同步422通道3记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(6),strTemp,ifcolor);
    ifcolor=0;
    */

    /// 6 数据输入状态 itemp=6,root=6
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],0))
    {
        strTemp = "PAL模拟视频输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "PAL模拟视频输入状态:无输入";
    }
    Tree_AddItem(item_root->child(6),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],1))
    {
        strTemp = "HD-SDI数字视频输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "HD-SDI数字视频输入状态:无输入";
    }
    Tree_AddItem(item_root->child(6),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],2))
    {
        strTemp = "Camera Link通道1输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "Camera Link通道1输入状态:无输入";
    }
    Tree_AddItem(item_root->child(6),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],3))
    {
        strTemp = "Camera Link通道2输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "Camera Link通道2输入状态:无输入";
    }
    Tree_AddItem(item_root->child(6),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],4))
    {
        strTemp = "同步422通道1输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道1输入状态:无输入";
    }
    Tree_AddItem(item_root->child(6),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],5))
    {
        strTemp = "同步422通道2输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道2输入状态:无输入";
    }
    Tree_AddItem(item_root->child(6),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],6))
    {
        strTemp = "同步422通道3输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道3输入状态:无输入";
    }
    Tree_AddItem(item_root->child(6),strTemp,ifcolor);
    ifcolor=0;


    /* 2018-12-18前版本
    //7 0x55 故障状态字1 itemp=7,root=7
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x03)) == 0x01){
        strTemp = "PAL模拟视频数据采集故障:NO";
    }else if((((u_char)readAllData[iTemp]) & (0x03)) == 0x03){
        strTemp = "PAL模拟视频数据采集故障:YES";
        ifcolor = 1;
    }else{
        strTemp = "PAL模拟视频数据采集故障:无效";
        ifcolor = 2;
    }

    Tree_AddItem(item_root->child(7),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x0C)) == 0x04){
        strTemp = "HD-SDI数字视频数据采集故障:NO";
    }else if((((u_char)readAllData[iTemp]) & (0x0C)) == 0x0C){
        strTemp = "HD-SDI数字视频数据采集故障:YES";
        ifcolor = 1;
    }else{
        strTemp = "HD-SDI数字视频数据采集故障:无效";
        ifcolor = 2;
    }

    Tree_AddItem(item_root->child(7),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x30)) == 0x10){
        strTemp = "Camera Link通道1数据采集故障:NO";
    }else if((((u_char)readAllData[iTemp]) & (0x30)) == 0x30){
        strTemp = "Camera Link通道1数据采集故障:YES";
        ifcolor = 1;
    }else{
        strTemp = "Camera Link通道1数据采集故障:无效";
        ifcolor = 2;
    }

    Tree_AddItem(item_root->child(7),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0xC0)) == 0x40){
        strTemp = "Camera Link通道2数据采集故障:NO";
    }else if((((u_char)readAllData[iTemp]) & (0xC0)) == 0xC0){
        strTemp = "Camera Link通道2数据采集故障:YES";
        ifcolor = 1;
    }else{
        strTemp = "Camera Link通道2数据采集故障:无效";
        ifcolor = 2;
    }

    Tree_AddItem(item_root->child(7),strTemp,ifcolor);
    ifcolor=0;

    //8 0x55 故障状态字2 itemp=8,root=8
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x03)) == 0x01){
        strTemp = "同步422通道1数据采集故障:NO";
    }else if((((u_char)readAllData[iTemp]) & (0x03)) == 0x03){
        strTemp = "同步422通道1数据采集故障:YES";
        ifcolor = 1;
    }else{
        strTemp = "同步422通道1数据采集故障:无效";
        ifcolor = 2;
    }

    Tree_AddItem(item_root->child(8),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x0C)) == 0x04){
        strTemp = "同步422通道2数据采集故障:NO";
    }else if((((u_char)readAllData[iTemp]) & (0x0C)) == 0x0C){
        strTemp = "同步422通道2数据采集故障:YES";
        ifcolor = 1;
    }else{
        strTemp = "同步422通道2数据采集故障:无效";
        ifcolor = 2;
    }

    Tree_AddItem(item_root->child(8),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x30)) == 0x10){
        strTemp = "同步422通道3数据采集故障:NO";
    }else if((((u_char)readAllData[iTemp]) & (0x30)) == 0x30){
        strTemp = "同步422通道3数据采集故障:YES";
        ifcolor = 1;
    }else{
        strTemp = "同步422通道3数据采集故障:无效";
        ifcolor = 2;
    }

    Tree_AddItem(item_root->child(8),strTemp,ifcolor);
    ifcolor=0;

    //9 0x64  剩余容量(大区) itemp=9,root=9
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //10-11 0x07e2 大区最老数据时间-年 itemp=10,root=10
    iTemp++;
    strTemp.sprintf("0x%02x%02x:",(u_char)readAllData.at(iTemp),(u_char)readAllData.at(iTemp+1));
    iTemp++;
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //12 0x04 大区最老数据时间-月 itemp=11,root=11
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //13 0x04 大区最老数据时间-日 itemp=12,root=12
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //14 0x04 大区最老数据时间-时 itemp=13,root=13
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //15 0x04 大区最老数据时间-分 itemp=14,root=14
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //16 0x04 大区最老数据时间-秒  itemp=15,root=15
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;
    */

    //7   剩余容量(大区) itemp=7,root=7
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //8-9 0x07e2 大区最老数据时间-年 itemp=8,root=8
    iTemp++;
    strTemp.sprintf("0x%02x%02x:",(u_char)readAllData.at(iTemp),(u_char)readAllData.at(iTemp+1));
    iTemp++;
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //10 0x04 大区最老数据时间-月 itemp = 9,root = 9
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //11 0x04 大区最老数据时间-日  itemp = 10,root = 10
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //12 0x04 大区最老数据时间-时 itemp = 11,root = 11
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //13 0x04 大区最老数据时间-分  root = 12
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //14 0x04 大区最老数据时间-秒  root = 13
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //15~16 硬盘擦写次数  root = 14
    iTemp++;
    strTemp.sprintf("0x%02x%02x:",(u_char)readAllData.at(iTemp),(u_char)readAllData.at(iTemp+1));
    iTemp++;
    strTemp += g_strParaName[iTemp-2];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;


    /* 2018-12-18前版本
    //17 0x55 小区记录状态16
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x03)) == 0x01){
        strTemp = "PAL模拟视频小区记录状态:停止记录";
        //                ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0x03)) == 0x03){
        strTemp = "PAL模拟视频小区记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "PAL模拟视频小区记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(16),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x0C)) == 0x04){
        strTemp = "HD-SDI数字视频小区记录状态:停止记录";
        //                ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0x0C)) == 0x0C){
        strTemp = "HD-SDI数字视频小区记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "HD-SDI数字视频小区记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(16),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x30)) == 0x10){
        strTemp = "Camera Link通道1小区记录状态:停止记录";
        //                ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0x30)) == 0x30){
        strTemp = "Camera Link通道1小区记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "Camera Link通道1小区记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(16),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0xC0)) == 0x40){
        strTemp = "Camera Link通道2小区记录状态:停止记录";
        //                ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0xC0)) == 0xC0){
        strTemp = "Camera Link通道2小区记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "Camera Link通道2小区记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(16),strTemp,ifcolor);
    ifcolor=0;

    //18 0x55 小区记录状态2
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-1];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x03)) == 0x01){
        strTemp = "同步422通道1小区记录状态:停止记录";
        //                ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0x03)) == 0x03){
        strTemp = "同步422通道1小区记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "同步422通道1小区记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(17),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x0C)) == 0x04){
        strTemp = "同步422通道2小区记录状态:停止记录";
        //                ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0x0C)) == 0x0C){
        strTemp = "同步422通道2小区记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "同步422通道2小区记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(17),strTemp,ifcolor);
    ifcolor=0;

    if((((u_char)readAllData[iTemp]) & (0x30)) == 0x10){
        strTemp = "同步422通道3小区记录状态:停止记录";
        //                ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0x30)) == 0x30){
        strTemp = "同步422通道3小区记录状态:记录";
        ifcolor = 3;
    }else{
        strTemp = "同步422通道3小区记录状态:无效";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(17),strTemp,ifcolor);
    ifcolor=0;

    */

    /// 17 通信字状态  root = 15
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-2];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData.at(iTemp),0))
    {
        strTemp = "通信故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "通信正常";
    }
    Tree_AddItem(item_root->child(15),strTemp,ifcolor);
    ifcolor=0;

    /// 18 小区记录状态  root = 16
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-2];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],0))
    {
        strTemp = "PAL模拟视频小区记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "PAL模拟视频小区记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(16),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],1))
    {
        strTemp = "HD-SDI数字视频小区记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "HD-SDI数字视频小区记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(16),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],2))
    {
        strTemp = "Camera Link通道1小区记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "Camera Link通道1小区记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(16),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],3))
    {
        strTemp = "Camera Link通道2小区记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "Camera Link通道2小区记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(16),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],4))
    {
        strTemp = "同步422通道1小区记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道1小区记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(16),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],5))
    {
        strTemp = "同步422通道2小区记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道2小区记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(16),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)readAllData[iTemp],6))
    {
        strTemp = "同步422通道3小区记录状态:正常记录";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道3小区记录状态:停止记录";
    }
    Tree_AddItem(item_root->child(16),strTemp,ifcolor);
    ifcolor=0;


    //19 0x64 MAR剩余容量(小区)  root = 17
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-2];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //20-21 0x07e2 小区最老数据时间-年  root = 18
    iTemp++;
    strTemp.sprintf("0x%02x%02x:",(u_char)readAllData.at(iTemp),(u_char)readAllData.at(iTemp+1));
    iTemp++;
    strTemp += g_strParaName[iTemp-3];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //22 0x04 小区最老数据时间-月  root = 19
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-3];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //23 0x04 小区最老数据时间-日  root = 20
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-3];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //24 0x04 小区最老数据时间-时  root = 21
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-3];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //25 0x04 小区最老数据时间-分  root = 22
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-3];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //26 0x04 小区最老数据时间-秒  root = 23
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-3];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //27 0x01主版本号   root = 24
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-3];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //28 0x02 次版本号  root = 25
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-3];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //29 0x00 备份  root = 26
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-3];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //30  root = 27
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));

    ui->label_p_count->setText("AA 55 : "+strTemp);

    strTemp += g_strParaName[iTemp-3];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    //31  root = 28
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)readAllData.at(iTemp));
    strTemp += g_strParaName[iTemp-3];
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    if(IsTreeExpand)
        ui->treeCtrl->expandAll();
    return ;
}
void MainWindow::
To_ParseData_B5A9_PB85(QByteArray _data)
{
    // qDebug()<<_data.toHex(' ');
    if(CloseStatus)
        return;
    ui->treeARM->clear();
    //qDebug()<<_data.size();
    QTreeWidgetItem *item_root = new QTreeWidgetItem(ui->treeARM);//传递父对象

    item_root->setText(0,"调试信息结果集解析");

    int ifcolor=0;
    QString strTemp;

    QByteArray strT;
    /* strT = _data.mid(17,12);
    strTemp=strT.toHex(' ');
    //qDebug()<<"14:"<<_data[14].toHex;
    Tree_AddItem(item_root,strTemp,ifcolor);

    strT = _data.mid(29,12);
    strTemp=strT.toHex(' ');
    //qDebug()<<"14:"<<_data[14].toHex;
    Tree_AddItem(item_root,strTemp,ifcolor);*/

    strT = _data.mid(0,28);
    strTemp=strT.toHex(' ');
    //qDebug()<<"14:"<<_data[14].toHex;
    Tree_AddItem(item_root,strTemp,ifcolor);

    strT = _data.mid(28,29);
    strTemp=strT.toHex(' ');
    //qDebug()<<"14:"<<_data[14].toHex;
    Tree_AddItem(item_root,strTemp,ifcolor);

    int iTemp = 0;
    int iRoot = 0;









    QString a;

    //0 0xB5  帧同步码  1
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "帧同步码";

    Tree_AddItem(item_root,strTemp,ifcolor);


    //1 0xA9  帧同步码  2
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "帧同步码";

    Tree_AddItem(item_root,strTemp,ifcolor);
    //2 帧长  3
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "帧长";

    Tree_AddItem(item_root,strTemp,ifcolor);

    //3-4 编码版1代码版本
    iTemp++;
    iRoot++;
    ifcolor = 0;
    QString c_v=QString::number((int)_data.at(iTemp), 10);
    char a1 = (u_char)_data.at(iTemp);

    iTemp++;
    iRoot++;
    char a2 = (u_char)_data.at(iTemp);
    QString z_v=QString::number((int)_data.at(iTemp), 10);
    short releaseval = (a2 << 8) | a1;
    strTemp.sprintf("0x%02x:",releaseval);
    strTemp += "编码板代码版本";
    // qDebug()<<"1:v"<<z_v<<c_v;
    strTemp =strTemp+":V"+z_v+"."+c_v;
    ifcolor=4;
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;


    //5-6
    iTemp++;

    ifcolor = 0;
    c_v=QString::number((int)_data.at(iTemp), 10);
    a1 = (u_char)_data.at(iTemp);
    iTemp++;
    iRoot++;

    a2 = (u_char)_data.at(iTemp);
    z_v=QString::number((int)_data.at(iTemp), 10);
    short releaseva = (a2 << 8) | a1;
    strTemp.sprintf("0x%02x:",releaseva);
    strTemp += "编码板系统版本";
    // qDebug()<<"1:v"<<z_v<<c_v;
    strTemp =strTemp+":V"+z_v+"."+c_v;
    ifcolor=4;
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;


    //7-8 sdi1接口状态
    iTemp++;
    iRoot++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",((u_char)_data.at(iTemp) << 8) | (u_char)_data.at(iTemp+1));
    strTemp += "sdi1接口状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp = "工作状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0) && !checkbitValue((u_char)_data.at(iTemp),1))
    {

        strTemp += "无效";
    }
    else if(checkbitValue((u_char)_data.at(iTemp),0) && !checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "normal";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),0) && checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "mbit";
    }
    else
    {
        strTemp += "ibit";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    strTemp = "信号状态:";
    if(checkbitValue((u_char)_data.at(iTemp),2) && !checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp += "有效信息";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),2) && checkbitValue((u_char)_data.at(iTemp),3))
    {
        if(UseKey)
        {
            strTemp += "有效信息";
        }
        else
        {
            strTemp += "无有效信息";
        }

    }
    else
    {
        if(UseKey)
        {
            strTemp += "有效信息";
        }else
        {
            strTemp += "采集故障";
            ifcolor = 1;
        }

    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "数据发送状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),4))
    {
        if(UseKey)
        {
            strTemp += "发送";
        }
        else
        {
            strTemp += "未发送";
            ifcolor = 1;
        }

    }
    else
    {
        strTemp += "发送";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    ifcolor = 0;
    strTemp = "输入图像格式:";
    if(!checkbitValue((u_char)_data.at(iTemp),5)
            && !checkbitValue((u_char)_data.at(iTemp),6)
            &&  !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp += "无效";
        ifcolor = 1;
    }
    else if(checkbitValue((u_char)_data.at(iTemp),5)
            && !checkbitValue((u_char)_data.at(iTemp),6)
            &&  !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp += "PAL";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),5)
            && checkbitValue((u_char)_data.at(iTemp),6)
            &&  !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp += "720P";

    }
    else if(checkbitValue((u_char)_data.at(iTemp),5)
            && checkbitValue((u_char)_data.at(iTemp),6)
            &&  !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp += "1080P";
    }
    else
    {
        strTemp += "4K";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    iTemp++;
    ifcolor = 0;
    strTemp = "输入图像帧率:";
    if(!checkbitValue((u_char)_data.at(iTemp),0)
            && !checkbitValue((u_char)_data.at(iTemp),1)
            &&  !checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "无效";
        ifcolor = 1;
    }
    else if(checkbitValue((u_char)_data.at(iTemp),0)
            && !checkbitValue((u_char)_data.at(iTemp),1)
            &&  !checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "25";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),0)
            && checkbitValue((u_char)_data.at(iTemp),1)
            &&  !checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "30";

    }
    else if(checkbitValue((u_char)_data.at(iTemp),0)
            && checkbitValue((u_char)_data.at(iTemp),1)
            &&  !checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "50";
    }
    else
    {
        strTemp += "60";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "网络连接:";
    if(!checkbitValue((u_char)_data.at(iTemp),3))
    {
        if(UseKey)
        {
            strTemp += "连接";
        }
        else
        {
            strTemp += "断开";
            ifcolor = 1;
        }

    }
    else
    {
        strTemp += "连接";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "网络数据输入状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),4))
    {
        if(UseKey)
        {
            strTemp += "有输入";
        }
        else
        {
            strTemp += "无输入";
        }

    }
    else
    {
        strTemp += "有输入";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "视频时间戳开关:";
    if(!checkbitValue((u_char)_data.at(iTemp),5))
    {
        strTemp += "关";
    }
    else
    {
        strTemp += "开";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //7-8 编码版2代码版本
    // iTemp++;
    // iRoot++;


    // ifcolor = 0;
    // c_v=QString::number((int)_data.at(iTemp), 10);
    // char a11 = (u_char)_data.at(iTemp);

    //iTemp++;
    /* char a22 = (u_char)_data.at(iTemp);
    z_v=QString::number((int)_data.at(iTemp), 10);
    short releaseval1 = (a22 << 8) | a11;
    strTemp.sprintf("0x%02x:",releaseval1);
    strTemp += "编码板2代码版本";
    strTemp =strTemp+":V"+z_v+"."+c_v;

    Tree_AddItem(item_root,strTemp,ifcolor);*/


    //9-10 sdi2接口状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",((u_char)_data.at(iTemp) << 8) | (u_char)_data.at(iTemp+1));
    strTemp += "sdi2接口状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp = "工作状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0) && !checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "无效";
    }
    else if(checkbitValue((u_char)_data.at(iTemp),0) && !checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "normal";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),0) && checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "mbit";
    }
    else
    {
        strTemp += "ibit";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    strTemp = "信号状态:";
    if(checkbitValue((u_char)_data.at(iTemp),2) && !checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp += "有效信息";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),2) && checkbitValue((u_char)_data.at(iTemp),3))
    {
        if(UseKey)
        {
            strTemp += "有效信息";
        }
        else
        {
            strTemp += "无有效信息";
        }

    }
    else
    {
        if(UseKey)
        {
            strTemp += "有效信息";
        }
        else
        {
            strTemp += "采集故障";
            ifcolor = 1;
        }

    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "数据发送状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),4))
    {
        if(UseKey)
        {
            strTemp += "发送";
        }
        else
        {
            strTemp += "未发送";
            ifcolor = 1;
        }

    }
    else
    {
        strTemp += "发送";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    ifcolor = 0;
    strTemp = "输入图像格式:";
    if(!checkbitValue((u_char)_data.at(iTemp),5)
            && !checkbitValue((u_char)_data.at(iTemp),6)
            &&  !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp += "无效";
        ifcolor = 1;
    }
    else if(checkbitValue((u_char)_data.at(iTemp),5)
            && !checkbitValue((u_char)_data.at(iTemp),6)
            &&  !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp += "PAL";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),5)
            && checkbitValue((u_char)_data.at(iTemp),6)
            &&  !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp += "720P";

    }
    else if(checkbitValue((u_char)_data.at(iTemp),5)
            && checkbitValue((u_char)_data.at(iTemp),6)
            &&  !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp += "1080P";
    }
    else
    {
        strTemp += "4K";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    iTemp++;
    ifcolor = 0;
    strTemp = "输入图像帧率:";
    if(!checkbitValue((u_char)_data.at(iTemp),0)
            && !checkbitValue((u_char)_data.at(iTemp),1)
            &&  !checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "无效";
        ifcolor = 1;
    }
    else if(checkbitValue((u_char)_data.at(iTemp),0)
            && !checkbitValue((u_char)_data.at(iTemp),1)
            &&  !checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "25";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),0)
            && checkbitValue((u_char)_data.at(iTemp),1)
            &&  !checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "30";

    }
    else if(checkbitValue((u_char)_data.at(iTemp),0)
            && checkbitValue((u_char)_data.at(iTemp),1)
            &&  !checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "50";
    }
    else
    {
        strTemp += "60";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "网络连接:";
    if(!checkbitValue((u_char)_data.at(iTemp),3))
    {
        if(UseKey)
        {
            strTemp += "连接";
        }
        else
        {

        }
        strTemp += "断开";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "网络数据输入状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),4))
    {
        if(UseKey)
        {
            strTemp += "有输入";
        }
        else
        {
            strTemp += "无输入";
//            ifcolor = 1;
        }

    }
    else
    {
        strTemp += "有输入";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "视频时间戳开关:";
    if(!checkbitValue((u_char)_data.at(iTemp),5))
    {
        strTemp += "关";
    }
    else
    {
        strTemp += "开";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);



    //11-12 主控板代码版本
    iTemp++;
    iRoot++;
    ifcolor = 0;
    c_v.clear();
    z_v.clear();

    u_char a111 = (u_char)_data.at(iTemp);
    c_v=QString::number((int)a111, 10);
    QByteArray aaaa;

    iTemp++;
    // iRoot++;
    u_char a222 = (u_char)_data.at(iTemp);
    aaaa[0] =_data.at(iTemp);

    z_v=QString::number((int)a222, 10);
    // qDebug()<<"2:"<<aaaa.toHex()<<z_v;
    //  short releaseval2 = (a222 << 8) | a111;
    strTemp.sprintf("0x%02x:",(a222 << 8) | a111);
    strTemp += "主控板代码版本";
    strTemp =strTemp+":V"+z_v+"."+c_v;
    ifcolor=4;
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;


    int  toiTemp = iTemp+44;

    ifcolor = 0;
    c_v=QString::number((int)_data.at(toiTemp), 10);
    a1 = (u_char)_data.at(toiTemp);
    toiTemp++;
    iRoot++;

    a2 = (u_char)_data.at(toiTemp);
    z_v=QString::number((int)_data.at(toiTemp), 10);
    releaseva = (a1<<8) | a2;
    strTemp.sprintf("0x%02x:",releaseva);
    strTemp += "控制板系统版本";
    // qDebug()<<"1:v"<<z_v<<c_v;
    strTemp =strTemp+":V"+c_v+"."+z_v;
    ifcolor=4;
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;



    //13 camlink主控板接口状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp+1));
    strTemp += "camlink主控板接口状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp = "工作状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0) && !checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "无效";
        ifcolor = 1;
    }
    else if(checkbitValue((u_char)_data.at(iTemp),0) && !checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "normal";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),0) && checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "mbit";
    }
    else
    {
        strTemp += "ibit";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "访问pcie设备:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "失败";
        ifcolor = 1;
    }
    else
    {
        strTemp += "成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "pcie通信串口状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp += "故障";
        ifcolor = 1;
    }
    else
    {
        strTemp += "正常";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "camlink数据输入状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),4))
    {
        strTemp += "无输入";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有输入";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "sync1数据输入状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),5))
    {
        strTemp += "无输入";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有输入";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "sync2数据输入状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),6))
    {
        strTemp += "无输入";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有输入";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "sync3数据输入状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp += "无输入";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有输入";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    //14
    iTemp++;
    strTemp = "sync4数据输入状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "无输入";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有输入";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    // qDebug()<<"it13:"<<iTemp;
    //15 FPGA_VER 数据4
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "FPGA软件版号:V";

    int b_b= _data.at(iTemp)&0xf;
    int a_a= (_data.at(iTemp)&0xf0)>>4;

    QString str;
    str.sprintf("%d.%d",a_a,b_b);
    strTemp += str;
    ifcolor=4;
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor=0;

    int toitemp;
    quint8 cor_num;
    //16  camlink  数据错误个数 数据5
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "camlink数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr缓存前错误个数:";
    a.clear();
    //QByte cor;
    //cor[0]=_data.at(iTemp);
    cor_num=_data.at(iTemp);
    a=QString::number(cor_num,10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    toitemp = iTemp+5;
    strTemp = "ddr缓存后错误个数:";
    a.clear();
    cor_num=_data.at(toitemp);
    a=QString::number(cor_num,10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //17同步串口1  数据错误个数 数据6
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口1数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr缓存前错误个数:";
    a.clear();
    cor_num=_data.at(iTemp);
    a=QString::number(cor_num,10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    //qDebug()<<"it16:"<<iTemp;

    toitemp = iTemp+5;
    strTemp = "ddr缓存后错误个数:";
    a.clear();
    cor_num=_data.at(toitemp);
    a=QString::number(cor_num,10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //18同步串口2  数据错误个数 数据7
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口2数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr缓存前错误个数:";
    a.clear();
    cor_num=_data.at(iTemp);
    a=QString::number(cor_num,10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    toitemp = iTemp+5;
    strTemp = "ddr缓存后错误个数:";
    a.clear();
    cor_num=_data.at(toitemp);
    a=QString::number(cor_num,10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //19同步串口3  数据错误个数 数据8
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口3数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr缓存前错误个数:";
    a.clear();
    cor_num=_data.at(iTemp);
    a=QString::number(cor_num,10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    toitemp = iTemp+5;
    strTemp = "ddr缓存后错误个数:";
    a.clear();
    cor_num=_data.at(toitemp);
    a=QString::number(cor_num,10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //20同步串口4  数据错误个数 数据9
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口4数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr缓存前错误个数:";
    a.clear();
    cor_num=_data.at(iTemp);
    a=QString::number(cor_num,10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    toitemp = iTemp+5;
    strTemp = "ddr缓存后错误个数:";
    a.clear();
    cor_num=_data.at(toitemp);
    a=QString::number(cor_num,10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //21 camlink  数据错误个数 数据10
    iTemp++;


    //22同步串口1  数据错误个数 数据11
    iTemp++;


    //23同步串口2  数据错误个数 数据12
    iTemp++;


    //24同步串口3  数据错误个数 数据13
    iTemp++;


    //25同步串口4  数据错误个数  数据14
    iTemp++;

    //26arm数据 数据错误个数 数据15
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步4下传数据错误个数";//同步4下传数据错误个数
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "接收数据错误个数:";
    a.clear();
    cor_num=_data.at(iTemp);
    a=QString::number(cor_num,10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    // qDebug()<<"it25:"<<iTemp;

    //27arm数据 数据错误个数 数据16

    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "4m数据个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr缓存 camlink 4m数据个数:";
    b_b= _data.at(iTemp)&0xf;
    a_a= (_data.at(iTemp)&0xf0)>>4;
    strTemp += QString::number(b_b,10)+"个";
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    strTemp = "同步串口1 camlink 4m数据个数:";
    strTemp += QString::number(a_a,10)+"个";;

    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    // qDebug()<<"it26:"<<iTemp;

    //28arm数据 数据错误个数 数据17
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "4m数据个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "同步串口2 camlink 4m数据个数:";
    b_b= _data.at(iTemp)&0xf;
    a_a= (_data.at(iTemp)&0xf0)>>4;
    strTemp += QString::number(b_b,10)+"个";
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    strTemp = "同步串口3 camlink 4m数据个数:";
    strTemp += QString::number(a_a,10)+"个";;

    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //29arm数据 数据错误个数 数据18
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "4m数据个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "同步串口4 camlink 4m数据个数:";
    b_b= _data.at(iTemp)&0xf;
    a_a= (_data.at(iTemp)&0xf0)>>4;
    strTemp += QString::number(b_b,10)+"个";
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    strTemp = "ARM数据已满个数:";
    strTemp += QString::number(a_a,10)+"个";;

    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    // qDebug()<<"it28:"<<iTemp;

    //30 数据有效性 19
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "数据有效性";
    Tree_AddItem(item_root,strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口1接收数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "无效数据";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有效数据";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口2接收数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "无效数据";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有效数据";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口3接收数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "无效数据";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有效数据";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口4接收数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp += "无效数据";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有效数据";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "camlink接收数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),4))
    {
        strTemp += "无效数据";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有效数据";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口4回传数据状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),5))
    {
        strTemp += "无效数据";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有效数据";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //31 链路连接状态 20
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "链路连接状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口1链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口2链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口3链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口4链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "camlink链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),4))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "pice链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),5))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "ddr链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),6))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //32 同步串口回传状态 21
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口回传状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口1回传状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "停止回传";
        ifcolor = 1;
    }
    else
    {
        strTemp += "回传中";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口2回传状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "停止回传";
        ifcolor = 1;
    }
    else
    {
        strTemp += "回传中";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口3回传状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "停止回传";
        ifcolor = 1;
    }
    else
    {
        strTemp += "回传中";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //33 ARM是否接收或者回传 22
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "ARM是否接收";
    Tree_AddItem(item_root,strTemp,ifcolor);

    ifcolor = 0;
    /* strTemp = "ARM是否要求回传同步串口1数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "停止回传";
        ifcolor = 1;
    }
    else
    {
        strTemp += "回传";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "ARM是否要求回传同步串口2数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "停止回传";
        ifcolor = 1;
    }
    else
    {
        strTemp += "回传";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "ARM是否要求回传同步串口3数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "停止回传";
        ifcolor = 1;
    }
    else
    {
        strTemp += "回传";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "ARM是否要求回传同步串口4数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp += "停止回传";
        ifcolor = 1;
    }
    else
    {
        strTemp += "回传";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);*/

    ifcolor = 0;
    strTemp = "ARM接收camlink数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),4))
    {
        strTemp += "停止接收";
        ifcolor = 1;
    }
    else
    {
        strTemp += "接收";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "ARM接收同步串口1数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),5))
    {
        strTemp += "停止接收";
        ifcolor = 1;
    }
    else
    {
        strTemp += "接收";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "ARM接收同步串口2数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),6))
    {
        strTemp += "停止接收";
        ifcolor = 1;
    }
    else
    {
        strTemp += "接收";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "ARM接收同步串口3数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp += "停止接收";
        ifcolor = 1;
    }
    else
    {
        strTemp += "接收";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //34 ARM是否接收或者回传 23
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "ARM是否接收";
    Tree_AddItem(item_root,strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "ARM接收同步串口4数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "停止接收";
        ifcolor = 1;
    }
    else
    {
        strTemp += "接收";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    /*ifcolor = 0;
    strTemp = "ARM是否给FPGA复位信号:";
    if(!checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "否";
        ifcolor = 1;
    }
    else
    {
        strTemp += "是";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    qDebug()<<"it33:"<<iTemp;*/




















    //14  FPGA_VER
    /* iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "FPGA软件版号";

       int b_b= _data.at(iTemp)&0xf;
       int a_a= (_data.at(iTemp)&0xf0)>>4;

    QString str;
    str.sprintf("%d.%d",a_a,b_b);
    strTemp += str;
    Tree_AddItem(item_root,strTemp,ifcolor);


    //15  camlink状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "camlink状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否有数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "链路没数据";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路有数据";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否传给上位机:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "链路不传给上位机";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路传给上位机";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否传给ARM:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "链路不传给ARM";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路传给ARM";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //16同步串口1状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口1状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否有数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "链路没数据";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路有数据";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否传给上位机:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "链路不传给上位机";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路传给上位机";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否传给ARM:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "链路不传给ARM";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路传给ARM";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //17同步串口2状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口2状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否有数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "链路没数据";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路有数据";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否传给上位机:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "链路不传给上位机";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路传给上位机";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否传给ARM:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "链路不传给ARM";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路传给ARM";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //18同步串口3状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口3状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否有数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "链路没数据";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路有数据";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否传给上位机:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "链路不传给上位机";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路传给上位机";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否传给ARM:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "链路不传给ARM";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路传给ARM";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //19同步串口4状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口4状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否有数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "链路没数据";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路有数据";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否传给上位机:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "链路不传给上位机";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路传给上位机";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路是否传给ARM:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "链路不传给ARM";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链路传给ARM";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //20ddr状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "ddr状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "链路连接状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "连接不成功";
        ifcolor = 1;
    }
    else
    {
        strTemp += "连接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "camlink ddr缓存满:";
    if(!checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "数据没满";
        ifcolor = 1;
    }
    else
    {
        strTemp += "数据满";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口1 ddr缓存满:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "数据没满";
        ifcolor = 1;
    }
    else
    {
        strTemp += "数据满";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口2 ddr缓存满:";
    if(!checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp += "数据没满";
        ifcolor = 1;
    }
    else
    {
        strTemp += "数据满";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口3 ddr缓存满:";
    if(!checkbitValue((u_char)_data.at(iTemp),4))
    {
        strTemp += "数据没满";
        ifcolor = 1;
    }
    else
    {
        strTemp += "数据满";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "同步串口4 ddr缓存满:";
    if(!checkbitValue((u_char)_data.at(iTemp),5))
    {
        strTemp += "数据没满";
        ifcolor = 1;
    }
    else
    {
        strTemp += "数据满";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //21ddr缓存 camlink 4m数据个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "ddr缓存 camlink 4m数据个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "个数:";
    QString a;
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //22ddr缓存 同步串口1  4m数据个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "ddr缓存-同步串口1-4m数据个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //23ddr缓存 同步串口2  4m数据个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "ddr缓存-同步串口2-4m数据个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //24ddr缓存 同步串口3  4m数据个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "ddr缓存-同步串口3-4m数据个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //25ddr缓存 同步串口4  4m数据个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "ddr缓存-同步串口4-4m数据个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //26camlink  数据错误个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "camlink数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr前错误个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //27同步串口1  数据错误个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口1数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr前错误个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //28同步串口2  数据错误个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口2数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr前错误个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //29同步串口3  数据错误个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口3数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr前错误个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //30同步串口4  数据错误个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口4数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr前错误个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);




    //31camlink  数据错误个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "camlink数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr后错误个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //32同步串口1  数据错误个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口1数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr后错误个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //33同步串口2  数据错误个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口2数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr后错误个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //34同步串口3  数据错误个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口3数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr后错误个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //35同步串口4  数据错误个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "同步串口4数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr后错误个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);







    //36arm数据 数据错误个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "arm数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr后错误个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //37arm数据 数据错误个数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "arm数据错误个数";
    Tree_AddItem(item_root,strTemp,ifcolor);
    ifcolor = 0;
    strTemp = "ddr后错误个数:";
    a.clear();
    a=QString::number(_data.at(iTemp),10)+"个";
    strTemp += a;
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);



    //38FPGA状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "FPGA状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "ARM发给FPGA链路有无数据:";
    if(!checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp += "无数据";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有数据";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "pcie初始化是否链接成功:";
    if(!checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "链接失败";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    strTemp = "ddr初始化是否链接成功:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "链接失败";
        ifcolor = 1;
    }
    else
    {
        strTemp += "链接成功";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);*/







    //35 异步422通道状态
    iTemp=iTemp+6;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "异步422通道1状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp = "工作状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),0) && !checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "0xFFFFFFFF";
    }
    else if(checkbitValue((u_char)_data.at(iTemp),0) && !checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "normal";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),0) && checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "mbit";
    }
    else
    {
        strTemp += "ibit";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    strTemp = "接口初始化:";
    if(!checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp += "失败";
        ifcolor = 1;
    }
    else
    {
        strTemp += "成功";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    strTemp = "数据状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp += "无输入";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    iRoot++;
    ifcolor = 0;
    strTemp = "异步422通道2状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp = "工作状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),4) && !checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "0xFFFFFFFF";
    }
    else if(checkbitValue((u_char)_data.at(iTemp),4) && !checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "normal";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),4) && checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp += "mbit";
    }
    else
    {
        strTemp += "ibit";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    strTemp = "接口初始化:";
    if(!checkbitValue((u_char)_data.at(iTemp),6))
    {
        strTemp += "失败";
        ifcolor = 1;
    }
    else
    {
        strTemp += "成功";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    strTemp = "数据状态:";
    if(!checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp += "无输入";
        ifcolor = 1;
    }
    else
    {
        strTemp += "有输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    // qDebug()<<"it39:"<<iTemp;
    // 41 存储阵列挂盘状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "存储阵列挂盘状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    // 42-45 存储阵列写盘错误次数
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp = "存储阵列写盘错误次数";
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp.sprintf("写盘sda错误次数:%d",(u_char)_data.at(iTemp));
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    iTemp++;
    strTemp.sprintf("写盘sdb错误次数:%d",(u_char)_data.at(iTemp));
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    iTemp++;
    strTemp.sprintf("写盘sdc错误次数:%d",(u_char)_data.at(iTemp));
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    iTemp++;
    strTemp.sprintf("写盘sdd错误次数:%d",(u_char)_data.at(iTemp));
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //46-47 并列阵列剩余容量
    iTemp++;
    iRoot++;
    ifcolor = 0;
    u_int capa = 0;//硬盘容量
    capa = (u_char)_data.at(iTemp);
    iTemp++;
    capa |= ((u_char)_data.at(iTemp) << 8);

    strTemp.sprintf("并列阵列剩余容量%d GB",capa);
    Tree_AddItem(item_root,strTemp,ifcolor);

    //48-49 串行阵列剩余容量
    iTemp++;
    iRoot++;
    ifcolor = 0;
    u_int val = 0;
    val = (u_char)_data.at(iTemp);
    iTemp++;
    val |= ((u_char)_data.at(iTemp) << 8);

    strTemp.sprintf("串行阵列剩余容量%d GB",val);
    Tree_AddItem(item_root,strTemp,ifcolor);
    //qDebug()<<"iTemp"<<iTemp;//52
    //压缩板串口关键数据50-53
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("压缩板串口关键数据:%02x %02x %02x %02x",(u_char)_data.at(iTemp),(u_char)_data.at(iTemp+1),(u_char)_data.at(iTemp+2),(u_char)_data.at(iTemp+3));
    Tree_AddItem(item_root,strTemp,ifcolor);

    //写入速度54
    iTemp += 4;
    iRoot++;
    ifcolor = 0;
    val = 0;
    val = (u_char)_data.at(iTemp);
    val += ((u_char)_data.at(iTemp + 1) << 8);
    strTemp.sprintf("写入速度:  %d MB/s",  val);
    Tree_AddItem(item_root,strTemp,ifcolor);

    iTemp++;
    //56-57


    if(IsTreeExpand)
        ui->treeARM->expandAll();

}




void MainWindow::To_ParseData_B5A9(QByteArray _data)
{
    ui->treeARM->clear();

    QTreeWidgetItem *item_root = new QTreeWidgetItem(ui->treeARM);//传递父对象

    item_root->setText(0,"调试信息结果集解析");

    int ifcolor=0;
    QString strTemp;
    int iTemp = 0;
    int iRoot = 0;

    //0 0xB5  帧同步码
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "帧同步码";

    Tree_AddItem(item_root,strTemp,ifcolor);


    //1 0xA9  帧同步码
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "帧同步码";

    Tree_AddItem(item_root,strTemp,ifcolor);

    //2 通信状态字
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "通信状态字";

    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp = "故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "正常";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //3 camerlink模块工作模式
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "camerlink模块工作模式";

    Tree_AddItem(item_root,strTemp,ifcolor);

    if((u_char)_data[iTemp] == 0x23){
        strTemp = "MBIT";
    }else if((u_char)_data[iTemp] == 0x25){
        strTemp = "IBIT";
    }else if((u_char)_data[iTemp] == 0x26){
        strTemp = "NORMAL";
        ifcolor = 0;
    }else{
        strTemp = "无效";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //4~7 camerlink模块通道1工作状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "camerlink模块通道1工作状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否收到记录命令：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否收到记录命令：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否有数据输入：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否有数据输入：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "数据采集是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据采集是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp],0))
    {
        strTemp = "数据存盘是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据存盘是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //8~11 camerlink模块通道2工作状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "camerlink模块通道2工作状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否收到记录命令：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否收到记录命令：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否有数据输入：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否有数据输入：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "数据采集是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据采集是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp],0))
    {
        strTemp = "数据存盘是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据存盘是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //12 camerlink模块pcie接口状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "PCIE链路连接状态";

    Tree_AddItem(item_root,strTemp,ifcolor);

    if((uchar)_data[iTemp] == 0x01)
    {
        strTemp = "故障";
        ifcolor = 1;
    }
    else if((uchar)_data[iTemp] == 0x00)
    {
        strTemp = "正常";
        ifcolor = 0;
    }
    else
    {
        strTemp = "无效值";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //13 压缩板串口状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "压缩板通信串口状态";

    Tree_AddItem(item_root,strTemp,ifcolor);
    if((uchar)_data[iTemp] == 0x01)
    {
        strTemp = "故障";
        ifcolor = 1;
    }
    else if((uchar)_data[iTemp] == 0x00)
    {
        strTemp = "正常";
        ifcolor = 0;
    }
    else
    {
        strTemp = "无效值";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //14 DDR初始化状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "压缩板DDR初始化状态";

    Tree_AddItem(item_root,strTemp,ifcolor);
    if((uchar)_data[iTemp] == 0x01)
    {
        strTemp = "故障";
        ifcolor = 1;
    }
    else if((uchar)_data[iTemp] == 0x00)
    {
        strTemp = "正常";
        ifcolor = 0;
    }
    else
    {
        strTemp = "无效值";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //15 压缩板温度
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "压缩板温度";

    Tree_AddItem(item_root,strTemp,ifcolor);


    //16 cam通道1工作状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "压缩板cam通道1工作状态";

    Tree_AddItem(item_root,strTemp,ifcolor);
    if((uchar)_data[iTemp] == 0x01)
    {
        strTemp = "记录启动";
        ifcolor = 2;
    }
    else if((uchar)_data[iTemp] == 0x00)
    {
        strTemp = "记录未启动";
        ifcolor = 0;
    }
    else
    {
        strTemp = "无效值";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //17 cam通道2工作状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "压缩板cam通道2工作状态";

    Tree_AddItem(item_root,strTemp,ifcolor);
    if((uchar)_data[iTemp] == 0x01)
    {
        strTemp = "记录启动";
        ifcolor = 2;
    }
    else if((uchar)_data[iTemp] == 0x00)
    {
        strTemp = "记录未启动";
        ifcolor = 0;
    }
    else
    {
        strTemp = "无效值";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //18 cam通道1输入数据正确性检测状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "压缩板cam通道1输入数据正确性检测";

    Tree_AddItem(item_root,strTemp,ifcolor);
    if((uchar)_data[iTemp] == 0x01)
    {
        strTemp = "数据错误";
        ifcolor = 1;
    }
    else if((uchar)_data[iTemp] == 0x00)
    {
        strTemp = "数据正确";
        ifcolor = 0;
    }
    else
    {
        strTemp = "无效";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //19 cam通道2输入数据正确性检测状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "压缩板cam通道2输入数据正确性检测";

    Tree_AddItem(item_root,strTemp,ifcolor);
    if((uchar)_data[iTemp] == 0x01)
    {
        strTemp = "数据错误";
        ifcolor = 1;
    }
    else if((uchar)_data[iTemp] == 0x00)
    {
        strTemp = "数据正确";
        ifcolor = 0;
    }
    else
    {
        strTemp = "无效";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //20 sync422模块工作模式
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "sync422模块工作模式";

    Tree_AddItem(item_root,strTemp,ifcolor);
    if((u_char)_data[iTemp] == 0x23){
        strTemp = "MBIT";
    }else if((u_char)_data[iTemp] == 0x25){
        strTemp = "IBIT";
    }else if((u_char)_data[iTemp] == 0x26){
        strTemp = "NORMAL";
        ifcolor = 0;
    }else{
        strTemp = "无效";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //21~24 sync422模块通道1工作状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "sync422模块通道1工作状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否收到记录命令：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否收到记录命令：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否有数据输入：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否有数据输入：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "数据采集是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据采集是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp],0))
    {
        strTemp = "数据存盘是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据存盘是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //25~28 sync422模块通道2工作状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "sync422模块通道2工作状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否收到记录命令：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否收到记录命令：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否有数据输入：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否有数据输入：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "数据采集是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据采集是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp],0))
    {
        strTemp = "数据存盘是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据存盘是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //29~32 sync422模块通道3工作状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "sync422模块通道3工作状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否收到记录命令：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否收到记录命令：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否有数据输入：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否有数据输入：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "数据采集是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据采集是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp],0))
    {
        strTemp = "数据存盘是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据存盘是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //33 sdi/pal模块工作模式
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "sdi/pal模块工作模式";

    Tree_AddItem(item_root,strTemp,ifcolor);
    if((u_char)_data[iTemp] == 0x23){
        strTemp = "MBIT";
    }else if((u_char)_data[iTemp] == 0x25){
        strTemp = "IBIT";
    }else if((u_char)_data[iTemp] == 0x26){
        strTemp = "NORMAL";
        ifcolor = 0;
    }else{
        strTemp = "无效";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //34~37 pal通道工作状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "pal模块通道工作状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否收到记录命令：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否收到记录命令：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否有数据输入：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否有数据输入：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "数据采集是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据采集是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp],0))
    {
        strTemp = "数据存盘是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据存盘是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //38~41 sdi通道工作状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "sdi模块通道工作状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否收到记录命令：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否收到记录命令：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "是否有数据输入：是";
        ifcolor = 2;
    }
    else
    {
        strTemp = "是否有数据输入：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp++],0))
    {
        strTemp = "数据采集是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据采集是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;

    if(checkbitValue((u_char)_data[iTemp],0))
    {
        strTemp = "数据存盘是否异常：是";
        ifcolor = 1;
    }
    else
    {
        strTemp = "数据存盘是否异常：否";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //42~51 存储阵列状态
    iTemp++;
    iRoot++;
    ifcolor = 0;
    strTemp = "存储阵列状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp.sprintf("挂盘个数：%d:",(u_char)_data.at(iTemp++));
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    strTemp.sprintf("挂盘状态：0x%2x:",(u_char)_data.at(iTemp++));
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    u_int capa;//硬盘容量

    capa = (u_char)_data.at(iTemp);
    iTemp++;
    capa |= ((u_char)_data.at(iTemp) << 8);

    strTemp.sprintf("硬盘总容量：%d GB",capa);
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    iTemp++;
    capa = (u_char)_data.at(iTemp);
    iTemp++;
    capa |= ((u_char)_data.at(iTemp) << 8);

    strTemp.sprintf("sdi大区剩余容量：%d GB",capa);
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    iTemp++;
    capa = (u_char)_data.at(iTemp);
    iTemp++;
    capa |= ((u_char)_data.at(iTemp) << 8);

    strTemp.sprintf("Cam大区剩余容量：%d GB",capa);
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    iTemp++;
    capa = (u_char)_data.at(iTemp);
    iTemp++;
    capa |= ((u_char)_data.at(iTemp) << 8);

    strTemp.sprintf("小区容量：%d GB",capa);
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    iTemp+= 3;



    if(IsTreeExpand)
        ui->treeARM->expandAll();
}


void MainWindow::To_ParseData_B5A9_20181228(QByteArray _data)
{
    ui->treeARM->clear();

#ifdef PB_57
    QByteArray readAllData(_data);
    QTreeWidgetItem *item_root = new QTreeWidgetItem(ui->treeARM);//传递父对象

    item_root->setText(0,"调试结果集解析");

    int ifcolor=0;
    QString strTemp;
    int iTemp = 0;

    //0
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //1
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //2 数管板状态1
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 0
    if((((u_char)readAllData[iTemp]) & (0x01)) == 0x01){
        strTemp = "数管板与存储板链路:连接正常";
    }else if((((u_char)readAllData[iTemp]) & (0x01)) == 0x00){
        strTemp = "数管板与存储板链路:连接失败";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 1
    if((((u_char)readAllData[iTemp]) & (0x02)) == 0x02){
        strTemp = "存储板:可用";
    }else if((((u_char)readAllData[iTemp]) & (0x02)) == 0x00){
        strTemp = "存储板:不可用";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 2
    if((((u_char)readAllData[iTemp]) & (0x04)) == 0x04){
        strTemp = "存储板格式化:正在格式化";
    }else if((((u_char)readAllData[iTemp]) & (0x04)) == 0x00){
        strTemp = "存储板格式化:未执行格式化";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit3
    if((((u_char)readAllData[iTemp]) & (0x08)) == 0x08){
        strTemp = "数管板读存储板操作:读操作";
    }else if((((u_char)readAllData[iTemp]) & (0x08)) == 0x00){
        strTemp = "数管板读存储板操作:空闲";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 4
    if((((u_char)readAllData[iTemp]) & (0x10)) == 0x10){
        strTemp = "数管板写存储板操作:写操作";
    }else if((((u_char)readAllData[iTemp]) & (0x10)) == 0x00){
        strTemp = "数管板写存储板操作:空闲";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 5
    if((((u_char)readAllData[iTemp]) & (0x20)) == 0x20){
        strTemp = "LBA是否可写:不可更新";
    }else if((((u_char)readAllData[iTemp]) & (0x20)) == 0x00){
        strTemp = "LBA是否可写:可更新";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit6
    if((((u_char)readAllData[iTemp]) & (0x40)) == 0x40){
        strTemp = "LBA数管初始化状态:初始化成功";
    }else if((((u_char)readAllData[iTemp]) & (0x40)) == 0x00){
        strTemp = "LBA数管初始化状态:初始化失败";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 7
    if((((u_char)readAllData[iTemp]) & (0x80)) == 0x80){
        strTemp = "disk_full:磁盘写满";
        ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0x80)) == 0x00){
        strTemp = "disk_full:磁盘未满";
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);


    //3 数管板状态2
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 0
    if((((u_char)readAllData[iTemp]) & (0x01)) == 0x01){
        strTemp = "part_disk_full:分区已写满";
        ifcolor = 1;
    }else if((((u_char)readAllData[iTemp]) & (0x01)) == 0x00){
        strTemp = "part_disk_full:分区未满";
    }

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 1
    if((((u_char)readAllData[iTemp]) & (0x02)) == 0x02){
        strTemp = "读文件列表结束标志:结束";
    }else if((((u_char)readAllData[iTemp]) & (0x02)) == 0x00){
        strTemp = "读文件列表结束标志:未结束";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //4 压缩板状态1
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 0
    if((((u_char)readAllData[iTemp]) & (0x01)) == 0x01){
        strTemp = "依次对应ADV1~8压缩通道初始化状态位-bit0:正常";
    }else if((((u_char)readAllData[iTemp]) & (0x01)) == 0x00){
        strTemp = "依次对应ADV1~8压缩通道初始化状态位-bit0:不正常";
        ifcolor = 1;
    }

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 1
    if((((u_char)readAllData[iTemp]) & (0x02)) == 0x02){
        strTemp = "bit_1:正常";
    }else if((((u_char)readAllData[iTemp]) & (0x02)) == 0x00){
        strTemp = "bit_1:不正常";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 2
    if((((u_char)readAllData[iTemp]) & (0x04)) == 0x04){
        strTemp = "bit_2:正常";
    }else if((((u_char)readAllData[iTemp]) & (0x04)) == 0x00){
        strTemp = "bit_2:不正常";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit3
    if((((u_char)readAllData[iTemp]) & (0x08)) == 0x08){
        strTemp = "bit_3:正常";
    }else if((((u_char)readAllData[iTemp]) & (0x08)) == 0x00){
        strTemp = "bit_3:不正常";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 4
    if((((u_char)readAllData[iTemp]) & (0x10)) == 0x10){
        strTemp = "bit_4:正常";
    }else if((((u_char)readAllData[iTemp]) & (0x10)) == 0x00){
        strTemp = "bit_4:不正常";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 5
    if((((u_char)readAllData[iTemp]) & (0x20)) == 0x20){
        strTemp = "bit_5:正常";
    }else if((((u_char)readAllData[iTemp]) & (0x20)) == 0x00){
        strTemp = "bit_5:不正常";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit6
    if((((u_char)readAllData[iTemp]) & (0x40)) == 0x40){
        strTemp = "bit_6:正常";
    }else if((((u_char)readAllData[iTemp]) & (0x40)) == 0x00){
        strTemp = "bit_6:不正常";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 7
    if((((u_char)readAllData[iTemp]) & (0x80)) == 0x80){
        strTemp = "bit_7:正常";
    }else if((((u_char)readAllData[iTemp]) & (0x80)) == 0x00){
        strTemp = "bit_7:不正常";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //5 压缩板状态2
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 0
    if((((u_char)readAllData[iTemp]) & (0x01)) == 0x01){
        strTemp = "压缩和数管GTX连接:正常";
    }else if((((u_char)readAllData[iTemp]) & (0x01)) == 0x00){
        strTemp = "压缩和数管GTX连接:不正常";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 1
    if((((u_char)readAllData[iTemp]) & (0x02)) == 0x02){
        strTemp = "压缩M和M1压缩数据GTX连接:正常";
    }else if((((u_char)readAllData[iTemp]) & (0x02)) == 0x00){
        strTemp = "压缩M和M1压缩数据GTX连接:不正常";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 2
    if((((u_char)readAllData[iTemp]) & (0x04)) == 0x04){
        strTemp = "压缩板心跳:正常";
    }else if((((u_char)readAllData[iTemp]) & (0x04)) == 0x00){
        strTemp = "压缩板心跳:不正常";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    Tree_AddItem(item_root->child(iTemp),"bit_3-7:保留",ifcolor);

    //6 压缩板状态3
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 0
    if((((u_char)readAllData[iTemp]) & (0x01)) == 0x01){
        strTemp = "ADV212 DREQ状态bit0:1";
    }else if((((u_char)readAllData[iTemp]) & (0x01)) == 0x00){
        strTemp = "ADV212 DREQ状态bit0:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 1
    if((((u_char)readAllData[iTemp]) & (0x02)) == 0x02){
        strTemp = "bit_1:1";
    }else if((((u_char)readAllData[iTemp]) & (0x02)) == 0x00){
        strTemp = "bit_1:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 2
    if((((u_char)readAllData[iTemp]) & (0x04)) == 0x04){
        strTemp = "bit_2:1";
    }else if((((u_char)readAllData[iTemp]) & (0x04)) == 0x00){
        strTemp = "bit_2:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit3
    if((((u_char)readAllData[iTemp]) & (0x08)) == 0x08){
        strTemp = "bit_3:1";
    }else if((((u_char)readAllData[iTemp]) & (0x08)) == 0x00){
        strTemp = "bit_3:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 4
    if((((u_char)readAllData[iTemp]) & (0x10)) == 0x10){
        strTemp = "bit_4:1";
    }else if((((u_char)readAllData[iTemp]) & (0x10)) == 0x00){
        strTemp = "bit_4:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 5
    if((((u_char)readAllData[iTemp]) & (0x20)) == 0x20){
        strTemp = "bit_5:1";
    }else if((((u_char)readAllData[iTemp]) & (0x20)) == 0x00){
        strTemp = "bit_5:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit6
    if((((u_char)readAllData[iTemp]) & (0x40)) == 0x40){
        strTemp = "bit_6:1";
    }else if((((u_char)readAllData[iTemp]) & (0x40)) == 0x00){
        strTemp = "bit_6:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 7
    if((((u_char)readAllData[iTemp]) & (0x80)) == 0x80){
        strTemp = "bit_7:1";
    }else if((((u_char)readAllData[iTemp]) & (0x80)) == 0x00){
        strTemp = "bit_7:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //7 压缩通道状态1
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 0
    if((((u_char)readAllData[iTemp]) & (0x01)) == 0x01){
        strTemp = "PL1压缩通道设置状态1-Bit0通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x01)) == 0x00){
        strTemp = "PL1压缩通道设置状态1-Bit0通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 1
    if((((u_char)readAllData[iTemp]) & (0x02)) == 0x02){
        strTemp = "bit_1通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x02)) == 0x00){
        strTemp = "bit_1通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 2
    if((((u_char)readAllData[iTemp]) & (0x04)) == 0x04){
        strTemp = "bit_2通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x04)) == 0x00){
        strTemp = "bit_2通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit3
    if((((u_char)readAllData[iTemp]) & (0x08)) == 0x08){
        strTemp = "bit_3通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x08)) == 0x00){
        strTemp = "bit_3通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 4
    if((((u_char)readAllData[iTemp]) & (0x10)) == 0x10){
        strTemp = "bit_4通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x10)) == 0x00){
        strTemp = "bit_4通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 5
    if((((u_char)readAllData[iTemp]) & (0x20)) == 0x20){
        strTemp = "bit_5通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x20)) == 0x00){
        strTemp = "bit_5通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit6
    if((((u_char)readAllData[iTemp]) & (0x40)) == 0x40){
        strTemp = "bit_6通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x40)) == 0x00){
        strTemp = "bit_6通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 7
    if((((u_char)readAllData[iTemp]) & (0x80)) == 0x80){
        strTemp = "bit_7通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x80)) == 0x00){
        strTemp = "bit_7通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //8 压缩通道状态2
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 0
    if((((u_char)readAllData[iTemp]) & (0x01)) == 0x01){
        strTemp = "PL1压缩通道设置状态2-Bit0通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x01)) == 0x00){
        strTemp = "PL1压缩通道设置状态2-Bit0通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 1
    if((((u_char)readAllData[iTemp]) & (0x02)) == 0x02){
        strTemp = "bit_1通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x02)) == 0x00){
        strTemp = "bit_1通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 2
    if((((u_char)readAllData[iTemp]) & (0x04)) == 0x04){
        strTemp = "bit_2通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x04)) == 0x00){
        strTemp = "bit_2通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit3
    if((((u_char)readAllData[iTemp]) & (0x08)) == 0x08){
        strTemp = "bit_3通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x08)) == 0x00){
        strTemp = "bit_3通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 4
    if((((u_char)readAllData[iTemp]) & (0x10)) == 0x10){
        strTemp = "bit_4通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x10)) == 0x00){
        strTemp = "bit_4通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 5
    if((((u_char)readAllData[iTemp]) & (0x20)) == 0x20){
        strTemp = "bit_5通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x20)) == 0x00){
        strTemp = "bit_5通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit6
    if((((u_char)readAllData[iTemp]) & (0x40)) == 0x40){
        strTemp = "bit_6通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x40)) == 0x00){
        strTemp = "bit_6通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 7
    if((((u_char)readAllData[iTemp]) & (0x80)) == 0x80){
        strTemp = "bit_7通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x80)) == 0x00){
        strTemp = "bit_7通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //9 压缩通道状态3
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 0
    if((((u_char)readAllData[iTemp]) & (0x01)) == 0x01){
        strTemp = "PL2压缩通道设置状态1-Bit0通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x01)) == 0x00){
        strTemp = "PL2压缩通道设置状态1-Bit0通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 1
    if((((u_char)readAllData[iTemp]) & (0x02)) == 0x02){
        strTemp = "bit_1通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x02)) == 0x00){
        strTemp = "bit_1通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 2
    if((((u_char)readAllData[iTemp]) & (0x04)) == 0x04){
        strTemp = "bit_2通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x04)) == 0x00){
        strTemp = "bit_2通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit3
    if((((u_char)readAllData[iTemp]) & (0x08)) == 0x08){
        strTemp = "bit_3通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x08)) == 0x00){
        strTemp = "bit_3通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 4
    if((((u_char)readAllData[iTemp]) & (0x10)) == 0x10){
        strTemp = "bit_4通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x10)) == 0x00){
        strTemp = "bit_4通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 5
    if((((u_char)readAllData[iTemp]) & (0x20)) == 0x20){
        strTemp = "bit_5通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x20)) == 0x00){
        strTemp = "bit_5通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit6
    if((((u_char)readAllData[iTemp]) & (0x40)) == 0x40){
        strTemp = "bit_6通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x40)) == 0x00){
        strTemp = "bit_6通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 7
    if((((u_char)readAllData[iTemp]) & (0x80)) == 0x80){
        strTemp = "bit_7通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x80)) == 0x00){
        strTemp = "bit_7通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //10 压缩通道状态4
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 0
    if((((u_char)readAllData[iTemp]) & (0x01)) == 0x01){
        strTemp = "PL2压缩通道设置状态2-Bit0通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x01)) == 0x00){
        strTemp = "PL2压缩通道设置状态2-Bit0通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 1
    if((((u_char)readAllData[iTemp]) & (0x02)) == 0x02){
        strTemp = "bit_1通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x02)) == 0x00){
        strTemp = "bit_1通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 2
    if((((u_char)readAllData[iTemp]) & (0x04)) == 0x04){
        strTemp = "bit_2通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x04)) == 0x00){
        strTemp = "bit_2通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit3
    if((((u_char)readAllData[iTemp]) & (0x08)) == 0x08){
        strTemp = "bit_3通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x08)) == 0x00){
        strTemp = "bit_3通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 4
    if((((u_char)readAllData[iTemp]) & (0x10)) == 0x10){
        strTemp = "bit_4通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x10)) == 0x00){
        strTemp = "bit_4通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 5
    if((((u_char)readAllData[iTemp]) & (0x20)) == 0x20){
        strTemp = "bit_5通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x20)) == 0x00){
        strTemp = "bit_5通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit6
    if((((u_char)readAllData[iTemp]) & (0x40)) == 0x40){
        strTemp = "bit_6通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x40)) == 0x00){
        strTemp = "bit_6通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 7
    if((((u_char)readAllData[iTemp]) & (0x80)) == 0x80){
        strTemp = "bit_7通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x80)) == 0x00){
        strTemp = "bit_7通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //11 压缩通道状态5
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 0
    if((((u_char)readAllData[iTemp]) & (0x01)) == 0x01){
        strTemp = "PL3压缩通道设置状态1-Bit0通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x01)) == 0x00){
        strTemp = "PL3压缩通道设置状态1-Bit0通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 1
    if((((u_char)readAllData[iTemp]) & (0x02)) == 0x02){
        strTemp = "bit_1通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x02)) == 0x00){
        strTemp = "bit_1通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 2
    if((((u_char)readAllData[iTemp]) & (0x04)) == 0x04){
        strTemp = "bit_2通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x04)) == 0x00){
        strTemp = "bit_2通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit3
    if((((u_char)readAllData[iTemp]) & (0x08)) == 0x08){
        strTemp = "bit_3通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x08)) == 0x00){
        strTemp = "bit_3通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 4
    if((((u_char)readAllData[iTemp]) & (0x10)) == 0x10){
        strTemp = "bit_4通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x10)) == 0x00){
        strTemp = "bit_4通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 5
    if((((u_char)readAllData[iTemp]) & (0x20)) == 0x20){
        strTemp = "bit_5通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x20)) == 0x00){
        strTemp = "bit_5通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit6
    if((((u_char)readAllData[iTemp]) & (0x40)) == 0x40){
        strTemp = "bit_6通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x40)) == 0x00){
        strTemp = "bit_6通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 7
    if((((u_char)readAllData[iTemp]) & (0x80)) == 0x80){
        strTemp = "bit_7通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x80)) == 0x00){
        strTemp = "bit_7通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //12 压缩通道状态6
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 0
    if((((u_char)readAllData[iTemp]) & (0x01)) == 0x01){
        strTemp = "PL3压缩通道设置状态2-Bit0通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x01)) == 0x00){
        strTemp = "PL3压缩通道设置状态2-Bit0通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 1
    if((((u_char)readAllData[iTemp]) & (0x02)) == 0x02){
        strTemp = "bit_1通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x02)) == 0x00){
        strTemp = "bit_1通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 2
    if((((u_char)readAllData[iTemp]) & (0x04)) == 0x04){
        strTemp = "bit_2通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x04)) == 0x00){
        strTemp = "bit_2通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit3
    if((((u_char)readAllData[iTemp]) & (0x08)) == 0x08){
        strTemp = "bit_3通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x08)) == 0x00){
        strTemp = "bit_3通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 4
    if((((u_char)readAllData[iTemp]) & (0x10)) == 0x10){
        strTemp = "bit_4通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x10)) == 0x00){
        strTemp = "bit_4通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 5
    if((((u_char)readAllData[iTemp]) & (0x20)) == 0x20){
        strTemp = "bit_5通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x20)) == 0x00){
        strTemp = "bit_5通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit6
    if((((u_char)readAllData[iTemp]) & (0x40)) == 0x40){
        strTemp = "bit_6通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x40)) == 0x00){
        strTemp = "bit_6通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 7
    if((((u_char)readAllData[iTemp]) & (0x80)) == 0x80){
        strTemp = "bit_7通道:1";
    }else if((((u_char)readAllData[iTemp]) & (0x80)) == 0x00){
        strTemp = "bit_7通道:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //13 压缩通道状态7
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit 0
    if((((u_char)readAllData[iTemp]) & (0x01)) == 0x01){
        strTemp = "M0M1-GTX数据:1";
    }else if((((u_char)readAllData[iTemp]) & (0x01)) == 0x00){
        strTemp = "M0M1-GTX数据:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 1
    if((((u_char)readAllData[iTemp]) & (0x02)) == 0x02){
        strTemp = "PL1有数据输入:1";
    }else if((((u_char)readAllData[iTemp]) & (0x02)) == 0x00){
        strTemp = "PL1有数据输入:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 2
    if((((u_char)readAllData[iTemp]) & (0x04)) == 0x04){
        strTemp = "PL1 Camerlink错误:1";
    }else if((((u_char)readAllData[iTemp]) & (0x04)) == 0x00){
        strTemp = "PL1 Camerlink错误:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit3
    if((((u_char)readAllData[iTemp]) & (0x08)) == 0x08){
        strTemp = "PL2有数据输入:1";
    }else if((((u_char)readAllData[iTemp]) & (0x08)) == 0x00){
        strTemp = "PL2有数据输入:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 4
    if((((u_char)readAllData[iTemp]) & (0x10)) == 0x10){
        strTemp = "PL2 Camerlink错误:1";
    }else if((((u_char)readAllData[iTemp]) & (0x10)) == 0x00){
        strTemp = "PL2 Camerlink错误:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit 5
    if((((u_char)readAllData[iTemp]) & (0x20)) == 0x20){
        strTemp = "PL3有数据输入:1";
    }else if((((u_char)readAllData[iTemp]) & (0x20)) == 0x00){
        strTemp = "PL3有数据输入:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit6
    if((((u_char)readAllData[iTemp]) & (0x40)) == 0x40){
        strTemp = "PL3 Camerlink错误:1";
    }else if((((u_char)readAllData[iTemp]) & (0x40)) == 0x00){
        strTemp = "PL3 Camerlink错误:0";
        //        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    Tree_AddItem(item_root->child(iTemp),"bit_7:保留",ifcolor);

    //14 压缩通道状态1
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp.sprintf("存储板1读写错误次数:0x%x",(((u_char)readAllData.at(iTemp)) & (0xf0)) >> 4);
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    strTemp.sprintf("存储板2读写错误次数:0x%x",((u_char)readAllData.at(iTemp)) & (0x0f));
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //15 压缩通道状态2
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp.sprintf("存储板3读写错误次数:0x%x",(((u_char)readAllData.at(iTemp)) & (0xf0)) >> 4);
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    strTemp.sprintf("存储板4读写错误次数:0x%x",((u_char)readAllData.at(iTemp)) & (0x0f));
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //16 压缩通道状态3
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp.sprintf("存储板5读写错误次数:0x%x",(((u_char)readAllData.at(iTemp)) & (0xf0)) >> 4);
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    strTemp.sprintf("存储板6读写错误次数:0x%x",((u_char)readAllData.at(iTemp)) & (0x0f));
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //17 压缩通道状态4
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp.sprintf("存储板7读写错误次数:0x%x",(((u_char)readAllData.at(iTemp)) & (0xf0)) >> 4);
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    strTemp.sprintf("存储板8读写错误次数:0x%x",((u_char)readAllData.at(iTemp)) & (0x0f));
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //18 数管板温度第一字节
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //19 数管板温度第二字节
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //20 数管板温度第三字节
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //21 数管板温度第四字节
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //22 压缩板温度第一字节
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //23 压缩板温度第二字节
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //24 压缩板温度第三字节
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //25 压缩板温度第四字节
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //26 存储盘工作状态1
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //27 存储盘工作状态2
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //28 存储盘工作状态3
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //29 存储盘工作状态4
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //30 盘1记录的之前的盘选择状态
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //31 盘2记录的之前的盘选择状态
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //32 盘3记录的之前的盘选择状态
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //33 盘4记录的之前的盘选择状态
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //34 盘5记录的之前的盘选择状态
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //35 盘6记录的之前的盘选择状态
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //36 盘7记录的之前的盘选择状态
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //37 盘8记录的之前的盘选择状态
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    //38 下载状态
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

    if((((u_char)readAllData.at(iTemp)) & (0x01)) == 0x01)
    {
        strTemp = "下载中";
        ifcolor = 2;
    }
    else
    {
        strTemp = "下载完成";
        ifcolor = 1;
    }

    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);


    //39 帧计数
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));

    ui->label_ARM_count->setText("99 66 : "+strTemp);

    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //40 校验码
    iTemp++;
    strTemp.sprintf("0x%02x",(u_char)readAllData.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];
    Tree_AddItem(item_root,strTemp,ifcolor);

#endif

#ifdef PB_60

    QTreeWidgetItem *item_root = new QTreeWidgetItem(ui->treeARM);//传递父对象

    item_root->setText(0,"调试信息结果集解析");

    int ifcolor=0;
    QString strTemp;
    int iTemp = 0;
    //0 0xB5  帧同步码
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);


    //1 0xA9  帧同步码
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //2
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit_4
    if((((u_char)_data.at(iTemp)) & (0x10)) == 0x10)
    {
        strTemp = "bit_4 = 1:初始化成功";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x10)) == 0x00)
    {
        strTemp = "bit_4 = 0:初始化失败";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_5
    if((((u_char)_data.at(iTemp)) & (0x20)) == 0x20)
    {
        strTemp = "bit_5 = 1:初始化成功";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x20)) == 0x00)
    {
        strTemp = "bit_5 = 0:初始化失败";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_6
    if((((u_char)_data.at(iTemp)) & (0x40)) == 0x40)
    {
        strTemp = "bit_6 = 1:初始化成功";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x40)) == 0x00)
    {
        strTemp = "bit_6 = 0:初始化失败";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_7
    if((((u_char)_data.at(iTemp)) & (0x80)) == 0x80)
    {
        strTemp = "bit_7 = 1:初始化成功";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x80)) == 0x00)
    {
        strTemp = "bit_7 = 0:初始化失败";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //3
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit_0
    if((((u_char)_data.at(iTemp)) & (0x01)) == 0x01)
    {
        strTemp = "bit_0 = 1:初始化成功";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x01)) == 0x00)
    {
        strTemp = "bit_0 = 0:初始化失败";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_1
    if((((u_char)_data.at(iTemp)) & (0x02)) == 0x02)
    {
        strTemp = "bit_1 = 1:初始化成功";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x02)) == 0x00)
    {
        strTemp = "bit_1 = 0:初始化失败";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_2
    if((((u_char)_data.at(iTemp)) & (0x04)) == 0x04)
    {
        strTemp = "bit_2 = 1:初始化成功";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x04)) == 0x00)
    {
        strTemp = "bit_2 = 0:初始化失败";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_3
    if((((u_char)_data.at(iTemp)) & (0x08)) == 0x08)
    {
        strTemp = "bit_3 = 1:初始化成功";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x08)) == 0x00)
    {
        strTemp = "bit_3 = 0:初始化失败";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_4
    if((((u_char)_data.at(iTemp)) & (0x10)) == 0x10)
    {
        strTemp = "bit_4 = 1:打开锁定成功";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x10)) == 0x00)
    {
        strTemp = "bit_4 = 0:打开锁定失败";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_6
    if((((u_char)_data.at(iTemp)) & (0x40)) == 0x40)
    {
        strTemp = "bit_6 = 1:打开锁定成功";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x40)) == 0x00)
    {
        strTemp = "bit_6 = 0:打开锁定失败";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //4
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit_3
    if((((u_char)_data.at(iTemp)) & (0x08)) == 0x08)
    {
        strTemp = "bit_3 = 1:压缩版心跳";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x08)) == 0x00)
    {
        strTemp = "bit_3 = 0:压缩版心跳";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //5
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit_0
    if((((u_char)_data.at(iTemp)) & (0x01)) == 0x01)
    {
        strTemp = "bit_0 = 1:DREQ状态正常";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x01)) == 0x00)
    {
        strTemp = "bit_0 = 0:DREQ状态不正常";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_1
    if((((u_char)_data.at(iTemp)) & (0x02)) == 0x02)
    {
        strTemp = "bit_1 = 1:DREQ状态正常";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x02)) == 0x00)
    {
        strTemp = "bit_1 = 0:DREQ状态不正常";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_2
    if((((u_char)_data.at(iTemp)) & (0x04)) == 0x04)
    {
        strTemp = "bit_2 = 1:DREQ状态正常";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x04)) == 0x00)
    {
        strTemp = "bit_2 = 0:DREQ状态不正常";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_3
    if((((u_char)_data.at(iTemp)) & (0x08)) == 0x08)
    {
        strTemp = "bit_3 = 1:DREQ状态正常";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x08)) == 0x00)
    {
        strTemp = "bit_3 = 0:DREQ状态不正常";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_4
    if((((u_char)_data.at(iTemp)) & (0x10)) == 0x10)
    {
        strTemp = "bit_4 = 1:DREQ状态正常";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x10)) == 0x00)
    {
        strTemp = "bit_4 = 0:DREQ状态不正常";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_5
    if((((u_char)_data.at(iTemp)) & (0x20)) == 0x20)
    {
        strTemp = "bit_5 = 1:DREQ状态正常";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x20)) == 0x00)
    {
        strTemp = "bit_5 = 0:DREQ状态不正常";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_6
    if((((u_char)_data.at(iTemp)) & (0x40)) == 0x40)
    {
        strTemp = "bit_6 = 1:DREQ状态正常";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x40)) == 0x00)
    {
        strTemp = "bit_6 = 0:DREQ状态不正常";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_7
    if((((u_char)_data.at(iTemp)) & (0x80)) == 0x80)
    {
        strTemp = "bit_7 = 1:DREQ状态正常";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x80)) == 0x00)
    {
        strTemp = "bit_7 = 0:DREQ状态不正常";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //6
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //7
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //    int _iTemp = (_data.at(iTemp-1)) + ((u_char)_data.at(iTemp)) * pow(16.0,2.0);
    int _iTemp = _data.at(iTemp);

    if(_iTemp >= 0)
    {
        _iTemp = _iTemp * pow(16.0,2.0) + _data.at(iTemp-1);
        _iTemp = _iTemp/16;
        strTemp.sprintf("压缩版温度:%d",_iTemp);
    }
    else
    {
        _iTemp = _iTemp * pow(16.0,2.0) - _data.at(iTemp-1);
        _iTemp = (~_iTemp)/16+1;
        strTemp.sprintf("压缩版温度:-%d",_iTemp);
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //8
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //9
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit_0
    if((((u_char)_data.at(iTemp-1)) & (0x01)) == 0x01)
    {
        strTemp = "bit_0 = 1:PL1压缩启动";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp-1)) & (0x01)) == 0x00)
    {
        strTemp = "bit_0 = 0:PL1压缩未启动";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_1-15
    int _iRate = ((u_char)_data.at(8)) + (_data.at(9)) * pow(16.0,2.0);
    _iRate = _iRate/2;
    if(_iRate == 0)
    {
        strTemp = "PL1不压缩";
        ifcolor = 0;
    }
    else
    {
        strTemp.sprintf("PL1压缩比=1:%d",_iRate);
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //10
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //11
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit_0
    if((((u_char)_data.at(iTemp-1)) & (0x01)) == 0x01)
    {
        strTemp = "bit_0 = 1:PL2压缩启动";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp-1)) & (0x01)) == 0x00)
    {
        strTemp = "bit_0 = 0:PL2压缩未启动";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_1-15
    _iRate = ((u_char)_data.at(8)) + (_data.at(9)) * pow(16.0,2.0);
    _iRate = _iRate/2;
    if(_iRate == 0)
    {
        strTemp = "PL2不压缩";
        ifcolor = 0;
    }
    else
    {
        strTemp.sprintf("PL2压缩比=1:%d",_iRate);
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //12
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //13
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit_0
    if((((u_char)_data.at(iTemp-1)) & (0x01)) == 0x01)
    {
        strTemp = "bit_0 = 1:HSID压缩启动";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp-1)) & (0x01)) == 0x00)
    {
        strTemp = "bit_0 = 0:HSID压缩未启动";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_1-15
    _iRate = ((u_char)_data.at(8)) + (_data.at(9)) * pow(16.0,2.0);
    _iRate = _iRate/2;
    if(_iRate == 0)
    {
        strTemp = "HSID不压缩";
        ifcolor = 0;
    }
    else
    {
        strTemp.sprintf("HSID压缩比=1:%d",_iRate);
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //14
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //15
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit_0
    if((((u_char)_data.at(iTemp-1)) & (0x01)) == 0x01)
    {
        strTemp = "bit_0 = 1:LFCCD压缩启动";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp-1)) & (0x01)) == 0x00)
    {
        strTemp = "bit_0 = 0:LFCCD压缩未启动";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_1-15
    _iRate = ((u_char)_data.at(8)) + (_data.at(9)) * pow(16.0,2.0);
    _iRate = _iRate/2;
    if(_iRate == 0)
    {
        strTemp = "LFCCD不压缩";
        ifcolor = 0;
    }
    else
    {
        strTemp.sprintf("LFCCD压缩比=1:%d",_iRate);
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //16 压缩通道状态
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit_0
    if((((u_char)_data.at(iTemp)) & (0x01)) == 0x01)
    {
        strTemp = "bit_0 = 1:11所红外";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x01)) == 0x00)
    {
        strTemp = "bit_0 = 0:613所红外";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_1
    if((((u_char)_data.at(iTemp)) & (0x02)) == 0x02)
    {
        strTemp = "bit_1 = 1:搭载模式1";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x02)) == 0x00)
    {
        strTemp = "bit_1 = 0:搭载模式0";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_2
    if((((u_char)_data.at(iTemp)) & (0x04)) == 0x04)
    {
        strTemp = "bit_2 = 1:M0M1间GTX通讯有误";
        ifcolor = 1;
    }
    else if((((u_char)_data.at(iTemp)) & (0x04)) == 0x00)
    {
        strTemp = "bit_2 = 0:M0M1间GTX通讯正确";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //17
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit_0
    if((((u_char)_data.at(iTemp)) & (0x01)) == 0x01)
    {
        strTemp = "bit_0 = 1:PL1_CAM有数据输入";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x01)) == 0x00)
    {
        strTemp = "bit_0 = 0:PL1_CAM无数据输入";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_1
    if((((u_char)_data.at(iTemp)) & (0x02)) == 0x02)
    {
        strTemp = "bit_1 = 1:PL1_CAM数据有误";
        ifcolor = 1;
    }
    else if((((u_char)_data.at(iTemp)) & (0x02)) == 0x00)
    {
        strTemp = "bit_1 = 0:PL1_CAM数据无误";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_2
    if((((u_char)_data.at(iTemp)) & (0x04)) == 0x04)
    {
        strTemp = "bit_2 = 1:PL2_CAM有数据输入";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x04)) == 0x00)
    {
        strTemp = "bit_2 = 0:PL2_CAM无数据输入";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_3
    if((((u_char)_data.at(iTemp)) & (0x08)) == 0x08)
    {
        strTemp = "bit_3 = 1:PL1_CAM数据有误";
        ifcolor = 1;
    }
    else if((((u_char)_data.at(iTemp)) & (0x08)) == 0x00)
    {
        strTemp = "bit_3 = 0:PL1_CAM数据无误";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_4
    if((((u_char)_data.at(iTemp)) & (0x10)) == 0x10)
    {
        strTemp = "bit_4 = 1:PL3_CAM有数据输入";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp)) & (0x10)) == 0x00)
    {
        strTemp = "bit_4 = 0:PL3_CAM无数据输入";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_5
    if((((u_char)_data.at(iTemp)) & (0x20)) == 0x20)
    {
        strTemp = "bit_5 = 1:PL3_CAM数据有误";
        ifcolor = 1;
    }
    else if((((u_char)_data.at(iTemp)) & (0x20)) == 0x00)
    {
        strTemp = "bit_5 = 0:PL3_CAM数据无误";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //18
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //19
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit_0
    if((((u_char)_data.at(iTemp-1)) & (0x01)) == 0x01)
    {
        strTemp = "bit_0 = 1:SAR_I压缩启动";
        ifcolor = 0;
    }
    else if((((u_char)_data.at(iTemp-1)) & (0x01)) == 0x00)
    {
        strTemp = "bit_0 = 0:SAR_I压缩未启动";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_1-15
    _iRate = ((u_char)_data.at(8)) + (_data.at(9)) * pow(16.0,2.0);
    _iRate = _iRate/2;
    if(_iRate == 0)
    {
        strTemp = "SAR_I不压缩";
        ifcolor = 0;
    }
    else
    {
        strTemp.sprintf("SAR_I压缩比=1:%d",_iRate);
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //20
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    //bit_0-3
    if((((u_char)_data.at(iTemp)) & (0x0F)) == 0x01)
    {
        strTemp = "bit_0-3 = 0x1:CamerLink大区写盘状态错误";
        ifcolor = 1;
    }
    else if((((u_char)_data.at(iTemp)) & (0x0F)) == 0x00)
    {
        strTemp = "bit_0-3 = 0x0:CamerLink大区写盘状态正常";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //bit_4-7
    if((((u_char)_data.at(iTemp)) & (0xF0)) == 0x10)
    {
        strTemp = "bit_4-7 = 0x1:CamerLink小区写盘状态错误";
        ifcolor = 1;
    }
    else if((((u_char)_data.at(iTemp)) & (0xF0)) == 0x00)
    {
        strTemp = "bit_4-7 = 0x0:CamerLink小区写盘状态正常";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //21
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    if((u_char)_data.at(iTemp) == 0x00)
    {
        strTemp = "存在";
        ifcolor = 0;
    }
    else if((u_char)_data.at(iTemp) == 0x01)
    {
        strTemp = "不存在";
        ifcolor = 1;
    }
    Tree_AddItem(item_root->child(iTemp),strTemp,ifcolor);

    //22
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);



    //23 帧计数 添加
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)m_iRecvFramCnt++);
    strTemp += g_strArmParaNmae[iTemp];

    Tree_AddItem(item_root,strTemp,ifcolor);

    if(m_iRecvFramCnt == 256)
    {
        m_iRecvFramCnt = 0;
    }

#endif

    if(IsTreeExpand)
        ui->treeARM->expandAll();
}
void MainWindow::To_ParseData_C7A7(QByteArray _data)
{
    ui->treeCtrl->clear();

    QTreeWidgetItem *item_root = new QTreeWidgetItem(ui->treeCtrl);

    item_root->setText(0,"用户结果集解析");
    int ifcolor=0;//颜色控制
    QString strTemp;//
    int iTemp = 0;//解析的第几字节
    int iRoot = 0;//树结构子节点计数
    //u_int tot;//硬盘总容量

    //0 0xc7  帧同步码
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "帧同步码";
    Tree_AddItem(item_root,strTemp,ifcolor);
    iRoot++;

    //1 0xa7  帧同步码
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "帧同步码";
    Tree_AddItem(item_root,strTemp,ifcolor);
    iRoot++;

    //2 消息字
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "消息字";
    Tree_AddItem(item_root,strTemp,ifcolor);
    iRoot++;

    //3 工作模式
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "工作模式";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if((u_char)_data[iTemp] == 0x23){
        strTemp = "MBIT";
    }else if((u_char)_data[iTemp] == 0x25){
        strTemp = "IBIT";
    }else if((u_char)_data[iTemp] == 0x28){
        strTemp = "安全擦除";
    } else if((u_char)_data[iTemp] == 0x26){
        strTemp = "NORMAL";
        ifcolor = 0;
    }else if((u_char)_data[iTemp] == 0x29){
        strTemp = "自毁模式";
        if(felfout == nullptr)
        {
            felfout = FelfOutTime::getInsantce();
            connect(this,SIGNAL(StartTo()),felfout,SLOT(starttime()));
            connect(felfout,SIGNAL(EndTime()),this,SLOT(InEndtime()));
        }

        if(checkbitValue((u_char)_data.at(21),0))
        {
            notself = false;
            felfout->show();
            emit StartTo();
        }
        else
        {
            ui->statusBar->showMessage(tr("物理自毁启动失败"));
        }

    }else{
        strTemp = "无效";
        ifcolor = 3;
    }

    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;
    iRoot++;

    //4 故障状态字1
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "故障状态字1";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp = "硬盘1故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘1故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp = "硬盘2故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘2故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp = "硬盘3故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘3故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp = "硬盘4故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "硬盘4故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor = 0;
    iRoot++;

    //5 故障状态字2
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "故障状态字2";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp = "HD-SDI通道1故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "HD-SDI通道1故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp = "HD-SDI通道2故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "HD-SDI通道2故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    /*if(checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp = "HD-SDI通道3故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "HD-SDI通道3故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp = "PAL记录故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "PAL记录故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);*/
    if(checkbitValue((u_char)_data.at(iTemp),4))
    {
        strTemp = "CAMERALINK记录故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "CAMERALINK记录故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),5))
    {
        strTemp = "同步RS422通道1记录故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "同步RS422通道1记录故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),6))
    {
        strTemp = "同步RS422通道2记录故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "同步RS422通道2记录故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp = "异步RS422通道1记录故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "异步RS422通道1记录故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);


    //6 故障状态字2
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));

    if(checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp = "模拟飞参数据记录故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "模拟飞参数据记录故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp = "网络通道记录故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "网络通道记录故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp = "同步RS422通道3记录故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "同步RS422通道3记录故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp = "同步RS422通道4记录故障状态:故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "同步RS422通道4记录故障状态:正常";
        ifcolor = 3;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    iRoot++;

    //7 8硬盘总容量
    iTemp++;
    u_int tot;//硬盘总容量
    tot = (u_char)_data.at(iTemp);
    iTemp++;
    tot |= ((u_char)_data.at(iTemp) << 8);
    strTemp.sprintf("0x%04x:",tot);
    strTemp += "硬盘总容量";
    Tree_AddItem(item_root,strTemp,ifcolor);

    /* 打印浮点型硬盘容量值（e.g 3.45T）*/
    strTemp.sprintf("%.2fT", tot/100.0);
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    iRoot++;

    //9 剩余容量
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "剩余容量";
    Tree_AddItem(item_root,strTemp,ifcolor);

    strTemp.sprintf("%d%%",(u_char)_data.at(iTemp));
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    iRoot++;

    //10 记录状态字
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "记录状态字";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp = "HD-SDI通道1记录状态:启动";
        ifcolor = 3;
    }
    else
    {
        strTemp = "HD-SDI通道1记录状态:停止";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp = "HD-SDI通道2记录状态:启动";
        ifcolor = 3;
    }
    else
    {
        strTemp = "HD-SDI通道2记录状态:停止";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    /*if(checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp = "HD-SDI通道3记录状态:启动";
        ifcolor = 3;
    }
    else
    {
        strTemp = "HD-SDI通道3记录状态:停止";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp = "PAL模拟视频记录状态:启动";
        ifcolor = 3;
    }
    else
    {
        strTemp = "PAL模拟视频记录状态:停止";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);*/
    if(checkbitValue((u_char)_data.at(iTemp),4))
    {
        strTemp = "CAMERALINK记录状态:启动";
        ifcolor = 3;
    }
    else
    {
        strTemp = "CAMERALINK记录状态:停止";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),5))
    {
        strTemp = "同步RS422通道1记录状态:启动";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步RS422通道1记录状态:停止";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),6))
    {
        strTemp = "同步RS422通道2记录状态:启动";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步RS422通道2记录状态:停止";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp = "异步RS422通道1记录状态:启动";
        ifcolor = 3;
    }
    else
    {
        strTemp = "异步RS422通道1记录状态:停止";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    //11 记录状态字
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    if(checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp = "模拟飞参数据记录状态:启动";
        ifcolor = 3;
    }
    else
    {
        strTemp = "模拟飞参数据记录状态:停止";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp = "网络通道记录状态:启动";
        ui->pushButton_bc->setEnabled(false);
        ifcolor = 3;
    }
    else
    {
        strTemp = "网络通道记录状态:停止";
        ui->pushButton_bc->setEnabled(true);
        //file.close();
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp = "同步RS422通道3记录状态:启动";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步RS422通道3记录状态:停止";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp = "同步RS422通道4记录状态:启动";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步RS422通道4记录状态:停止";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    iRoot++;

    //12 数据输入状态
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "数据输入状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp = "HD-SDI通道1数据输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "HD-SDI通道1数据输入状态:无输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp = "HD-SDI通道2数据输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "HD-SDI通道2数据输入状态:无输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    /*if(checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp = "HD-SDI通道3数据输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "HD-SDI通道3数据输入状态:无输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp = "PAL模拟视频数据输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "PAL模拟视频数据输入状态:无输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);*/
    if(checkbitValue((u_char)_data.at(iTemp),4))
    {
        strTemp = "CAMERALINK数据输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "CAMERALINK数据输入状态:无输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),5))
    {
        strTemp = "同步RS422通道1数据输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步RS422通道1数据输入状态:无输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),6))
    {
        strTemp = "同步RS422通道2数据输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步RS422通道2数据输入状态:无输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp = "异步RS422通道1数据输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "异步RS422通道1数据输入状态:无输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    //13 数据输入状态
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    if(checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp = "模拟飞参数据输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "模拟飞参数据输入状态:无输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp = "网络通道数据输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "网络通道数据输入状态:无输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp = "同步RS422通道3数据输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步RS422通道3数据输入状态:无输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp = "同步RS422通道4数据输入状态:有输入";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步RS422通道4数据输入状态:无输入";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor = 0;
    iRoot++;
    //14 sdi-1 编码参数
    iTemp++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "sdi-1 编码参数";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(((u_char)_data.at(iTemp) & 0x0F) == 1)
    {
        strTemp = "码率bit:4M";
    }
    else if(((u_char)_data.at(iTemp) & 0x0F) == 2)
    {
        strTemp = "码率bit:8M";
    }
    else if(((u_char)_data.at(iTemp) & 0x0F) == 4)
    {
        strTemp = "码率bit:12M";
    }
    else if(((u_char)_data.at(iTemp) & 0x0F) == 8)
    {
        strTemp = "码率bit:16M";
    }else if(((u_char)_data.at(iTemp) & 0x0F) == 5)
    {
        strTemp = "码率bit:10M";
    }
    else if(((u_char)_data.at(iTemp) & 0x0F) == 6)
    {
        strTemp = "码率bit:11M";
    }else
        strTemp = "码率bit:无效";
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),4)
            && !checkbitValue((u_char)_data.at(iTemp),5)
            && !checkbitValue((u_char)_data.at(iTemp),6)
            && !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp = "GOPbit:25";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),4)
            && checkbitValue((u_char)_data.at(iTemp),5)
            && !checkbitValue((u_char)_data.at(iTemp),6)
            && !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp = "GOPbit:30";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),4)
            && !checkbitValue((u_char)_data.at(iTemp),5)
            && checkbitValue((u_char)_data.at(iTemp),6)
            && !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp = "GOPbit:50";
    }
    else
    {
        strTemp = "GOPbit:60";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    iRoot++;

    //15 sdi-2 编码参数
    iTemp++;
    ifcolor = 0;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "sdi-2 编码参数";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(((u_char)_data.at(iTemp) & 0x0F) == 1)
    {
        strTemp = "码率bit:4M";
    }
    else if(((u_char)_data.at(iTemp) & 0x0F) == 2)
    {
        strTemp = "码率bit:8M";
    }
    else if(((u_char)_data.at(iTemp) & 0x0F) == 4)
    {
        strTemp = "码率bit:12M";
    }
    else if(((u_char)_data.at(iTemp) & 0x0F) == 8)
    {
        strTemp = "码率bit:16M";
    }else if(((u_char)_data.at(iTemp) & 0x0F) == 5)
    {
        strTemp = "码率bit:10M";
    }
    else if(((u_char)_data.at(iTemp) & 0x0F) == 6)
    {
        strTemp = "码率bit:11M";
    }else
        strTemp = "码率bit:无效";
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),4)
            && !checkbitValue((u_char)_data.at(iTemp),5)
            && !checkbitValue((u_char)_data.at(iTemp),6)
            && !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp = "GOPbit:25";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),4)
            && checkbitValue((u_char)_data.at(iTemp),5)
            && !checkbitValue((u_char)_data.at(iTemp),6)
            && !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp = "GOPbit:30";
    }
    else if(!checkbitValue((u_char)_data.at(iTemp),4)
            && !checkbitValue((u_char)_data.at(iTemp),5)
            && checkbitValue((u_char)_data.at(iTemp),6)
            && !checkbitValue((u_char)_data.at(iTemp),7))
    {
        strTemp = "GOPbit:50";
    }
    else
    {
        strTemp = "GOPbit:60";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    iRoot++;
    //    //16 sdi-3 编码参数
    iTemp++;
    //    ifcolor = 0;
    //    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    //    strTemp += "sdi-3 编码参数";
    //    Tree_AddItem(item_root,strTemp,ifcolor);

    //    if(((u_char)_data.at(iTemp) & 0x0F) == 1)
    //    {
    //        strTemp = "码率bit:4M";
    //    }
    //    else if(((u_char)_data.at(iTemp) & 0x0F) == 2)
    //    {
    //        strTemp = "码率bit:8M";
    //    }
    //    else if(((u_char)_data.at(iTemp) & 0x0F) == 4)
    //    {
    //        strTemp = "码率bit:12M";
    //    }
    //    else if(((u_char)_data.at(iTemp) & 0x0F) == 8)
    //    {
    //        strTemp = "码率bit:16M";
    //    }else if(((u_char)_data.at(iTemp) & 0x0F) == 5)
    //    {
    //        strTemp = "码率bit:10M";
    //    }
    //    else if(((u_char)_data.at(iTemp) & 0x0F) == 6)
    //    {
    //        strTemp = "码率bit:11M";
    //    }else
    //        strTemp = "码率bit:无效";
    //    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //    if(checkbitValue((u_char)_data.at(iTemp),4)
    //            && !checkbitValue((u_char)_data.at(iTemp),5)
    //            && !checkbitValue((u_char)_data.at(iTemp),6)
    //            && !checkbitValue((u_char)_data.at(iTemp),7))
    //    {
    //        strTemp = "GOPbit:25";
    //    }
    //    else if(!checkbitValue((u_char)_data.at(iTemp),4)
    //            && checkbitValue((u_char)_data.at(iTemp),5)
    //            && !checkbitValue((u_char)_data.at(iTemp),6)
    //            && !checkbitValue((u_char)_data.at(iTemp),7))
    //    {
    //        strTemp = "GOPbit:30";
    //    }
    //    else if(!checkbitValue((u_char)_data.at(iTemp),4)
    //            && !checkbitValue((u_char)_data.at(iTemp),5)
    //            && checkbitValue((u_char)_data.at(iTemp),6)
    //            && !checkbitValue((u_char)_data.at(iTemp),7))
    //    {
    //        strTemp = "GOPbit:50";
    //    }
    //    else
    //    {
    //        strTemp = "GOPbit:60";
    //    }
    //    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    //    iRoot++;
    //    //17 pal 编码参数
    iTemp++;
    //    ifcolor = 0;
    //    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    //    strTemp += "pal 编码参数";
    //    Tree_AddItem(item_root,strTemp,ifcolor);

    //    if(((u_char)_data.at(iTemp) & 0x0F) == 1)
    //    {
    //        strTemp = "码率bit:4M";
    //    }
    //    else if(((u_char)_data.at(iTemp) & 0x0F) == 2)
    //    {
    //        strTemp = "码率bit:8M";
    //    }
    //    else if(((u_char)_data.at(iTemp) & 0x0F) == 4)
    //    {
    //        strTemp = "码率bit:12M";
    //    }
    //    else if(((u_char)_data.at(iTemp) & 0x0F) == 8)
    //    {
    //        strTemp = "码率bit:16M";
    //    }else if(((u_char)_data.at(iTemp) & 0x0F) == 5)
    //    {
    //        strTemp = "码率bit:10M";
    //    }
    //    else if(((u_char)_data.at(iTemp) & 0x0F) == 6)
    //    {
    //        strTemp = "码率bit:11M";
    //    }else
    //        strTemp = "码率bit:无效";
    //    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //    if(checkbitValue((u_char)_data.at(iTemp),4)
    //            && !checkbitValue((u_char)_data.at(iTemp),5)
    //            && !checkbitValue((u_char)_data.at(iTemp),6)
    //            && !checkbitValue((u_char)_data.at(iTemp),7))
    //    {
    //        strTemp = "GOPbit:25";
    //    }
    //    else if(!checkbitValue((u_char)_data.at(iTemp),4)
    //            && checkbitValue((u_char)_data.at(iTemp),5)
    //            && !checkbitValue((u_char)_data.at(iTemp),6)
    //            && !checkbitValue((u_char)_data.at(iTemp),7))
    //    {
    //        strTemp = "GOPbit:30";
    //    }
    //    else if(!checkbitValue((u_char)_data.at(iTemp),4)
    //            && !checkbitValue((u_char)_data.at(iTemp),5)
    //            && checkbitValue((u_char)_data.at(iTemp),6)
    //            && !checkbitValue((u_char)_data.at(iTemp),7))
    //    {
    //        strTemp = "GOPbit:50";
    //    }
    //    else
    //    {
    //        strTemp = "GOPbit:60";
    //    }
    //    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    //    iRoot++;
    /*
    //15 通信状态字
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "通信状态字";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data[iTemp],0))
    {
        strTemp = "通信状态：故障";
        ifcolor = 1;
    }
    else
    {
        strTemp = "通信状态：正常";
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    ifcolor=0;
    iRoot++;
*/
    //18 版本号
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "版本号";
    Tree_AddItem(item_root,strTemp,ifcolor);

    int main_version = 0;//主版本号
    int second_version = 0;//次版本号
    for(int i = 0; i < 8 ;i++)
    {
        if(checkbitValue((u_char)_data[iTemp],i))
        {
            //表示为1
            if(i < 4)
                main_version += pow(2,i);
            else
                second_version += pow(2,i-4);
        }
    }
    strTemp = "主版本号:"+ QString::number(main_version,10);
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    strTemp = "次版本号:"+ QString::number(second_version,10);
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    iRoot++;
    //19 数据下传状态
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "数据下传状态";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(checkbitValue((u_char)_data.at(iTemp),0))
    {
        strTemp = "同步422通道1输出状态:下传中";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道1输出状态:停止下传";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),1))
    {
        strTemp = "同步422通道2输出状态:下传中";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道2输出状态:停止下传";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),2))
    {
        strTemp = "同步422通道3输出状态:下传中";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道3输出状态:停止下传";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);
    if(checkbitValue((u_char)_data.at(iTemp),3))
    {
        strTemp = "同步422通道4输出状态:下传中";
        ifcolor = 3;
    }
    else
    {
        strTemp = "同步422通道4输出状态:停止下传";
        ifcolor = 0;
    }
    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    //20 422传输速率
    iRoot++;
    iTemp++;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "422传输速率";
    Tree_AddItem(item_root,strTemp,ifcolor);

    if(((u_char)_data.at(iTemp) & 0x0F) == 1)
    {
        strTemp = "422传输速率:1M";
    }
    else if(((u_char)_data.at(iTemp) & 0x0F) == 2)
    {
        strTemp = "422传输速率:2M";
    }
    else if(((u_char)_data.at(iTemp) & 0x0F) == 3)
    {
        strTemp = "422传输速率:4M";
    }
    else if(((u_char)_data.at(iTemp) & 0x0F) == 4)
    {
        strTemp = "422传输速率:8M";
    }

    Tree_AddItem(item_root->child(iRoot),strTemp,ifcolor);

    ifcolor = 0;
    iRoot++;
    //23 校验码
    iTemp=23;
    strTemp.sprintf("0x%02x:",(u_char)_data.at(iTemp));
    strTemp += "校验码";
    Tree_AddItem(item_root,strTemp,ifcolor);
    iRoot++;


    if(IsTreeExpand)
        ui->treeCtrl->expandAll();

}

bool MainWindow::checkbitValue(uchar _value, int b_num)
{
    return ((_value>>b_num) & 0x01) == 0x01 ? true : false ;
}


void MainWindow::on_pushButton_17_clicked()
{
    m_bySendBigCmd= 0x48;
    SendRemoteData();

    QMessageBox::StandardButton rb = QMessageBox::question(NULL, "警 告", "确定要安全擦除?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if(rb == QMessageBox::Yes)
    {
        m_bySendBigCmd= 0x4D;
        SendRemoteData();
        ui->statusBar->showMessage(tr("安全擦除"));
    }

}

void MainWindow::on_btn_initcom_2_clicked()
{
    ui->label_13->setStyleSheet("background-color: rgb(0, 255, 0)");
    if (ui->cbox_coms_2->count() < 1)
    {
        if (QMessageBox::information(this, tr("提示"), tr("没有设置可用的串口信息"), QMessageBox::Retry | QMessageBox::Cancel) == QMessageBox::Retry)
        {
            GetAllCOMS();
        }
        else
        {
            return;
        }
    }

    if(!IsStopCom2)
    {
        //关闭串口
        IsStopCom2 = true;
        TM_sendData->stop();
        if(RS_sendData->isActive())
            RS_sendData->stop();
        my_serialport2->close();
        delete my_serialport2;
        my_serialport2 = NULL;
        // m_spSt = 0;
        // frame_num = 0;
        ui->statusBar->showMessage(tr("停止%1串口通信").arg(ui->cbox_coms_2->currentText()),3000);
        ui->btn_initcom_2->setText(tr("打开串口"));
        return ;
    }

    my_serialport2 = new Win_QextSerialPort(ui->cbox_coms_2->currentText(),QextSerialBase::EventDriven);

    if (my_serialport2->open(QIODevice::ReadWrite))
    {
        //设置波特率
        my_serialport2->setBaudRate(BAUD115200);
        //设置数据位
        my_serialport2->setDataBits(DATA_8);
        //设置校验位
        my_serialport2->setParity(PAR_NONE);
        //设置流控制
        my_serialport2->setFlowControl(FLOW_OFF);
        //设置停止位
        my_serialport2->setStopBits(STOP_1);
        ui->statusBar->showMessage(tr("%1初始化成功,串口通讯已开启").arg(ui->cbox_coms_2->currentText()),3000);
        connect(this->my_serialport,SIGNAL(readyRead()),this,SLOT(GetComDta()));
        ui->btn_initcom_2->setText(tr("关闭串口"));
    }
    else
    {
        QMessageBox::information(this, tr("提示"), tr("串口打开失败,请确认串口状态"), QMessageBox::Ok);
        return;
    }
    IsStopCom2 = false;
    ui->label_13->setStyleSheet("background-color: rgb(0, 255, 0)");
    TM_sendData->start(1000);
}


void MainWindow::on_btn_stop_cmd_2_clicked()
{
    if(IsStopCom2)
    {
        //QMessageBox::information(this, tr("提示"), tr("串口未初始化,请先初始化串口"), QMessageBox::Ok);
        //刷新一下串口
        ui->cbox_coms->clear();
        ui->cbox_coms_2->clear();
        ui->cbox_coms_3->clear();
        GetAllCOMS();
        return ;
    }
    else
    {
        //ui->statusBar->showMessage(tr("停止%1串口通信").arg(ui->cbox_coms->currentText()),3000);
        return;
    }
}

void MainWindow::on_pushButton_26_clicked()
{
    if(m_sendRS422)
    {
        m_sendRS422 = false;
        ui->pushButton_26->setText(tr("发送RS422"));
        if(RS_sendData->isActive())
            RS_sendData->stop();
    }
    else
    {
        m_sendRS422 = true;
        ui->pushButton_26->setText(tr("停止发送RS422"));
        RS_sendData->start(500);
    }
    rs422_frame_num = 0;
}

void MainWindow::on_pushButton_21_clicked()
{


    emit stopWritingSignal();
    emit Disconnect();
    ui->pushButton_21->setEnabled(false);
}

void MainWindow::on_pushButton_18_clicked()
{
    int filesize;
    QByteArray fileData;
    QString nn = QCoreApplication::applicationDirPath();
    filePath  = QFileDialog::getOpenFileName(nullptr, "Open File", nn, "All Files (*);;Text Files (*.bmp)");

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return;
    }else
    {
        QByteArray filetodata;
        filetodata = file.readAll();
        fileData = filetodata.mid(filenum,file.size()-filenum);
        filesize = fileData.size();
        file.close();
    }

    QByteArray headdata;
    QByteArray enddata;

    headdata.resize(63);
    headdata[0] = 0xe5;
    headdata[1] = 0x30;
    headdata[2] = 0x03;
    headdata[3] = 0xe2;
    headdata[4] = 0x00;
    headdata[5] = 0x23;

    int size =filesize + 53;
    headdata[6]=size&0xff;
    headdata[7]=size>>8&0xff;
    headdata[8]=size>>16&0xff;
    headdata[9]=size>>24&0xff;

    for(int i=0;i<53;i++)
    {
        headdata[10+i]=i;
    }

    enddata.resize(4);
    enddata[0] = 0x88;
    enddata[1] = 0xcc;
    enddata[2] = 0xdd;
    enddata[3] = 0xee;

    arry = headdata;
    arry += fileData;
    arry += enddata;
    //tcpthread->datathread(arry);
    tcpserver->set_data(arry);

    //    currentClient->write(arry);
    //    currentClient->flush();
    bOpenFile = true;


}

void MainWindow::on_pushButton_19_clicked()
{
    //StartData(5);
    m_bySendBigCmd = 0x24;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启同步422通道2记录"));
}

void MainWindow::on_pushButton_20_clicked()
{
    // StopData(5);
    m_bySendBigCmd = 0x27;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止同步422通道2记录"));
}

void MainWindow::on_pushButton_22_clicked()
{
    m_bySendBigCmd= 0x35;
    SendRemoteData();
    ui->statusBar->showMessage(tr("开启网络通道记录"));


    // udpSocket = new QUdpSocket(this);
    // udpSocket->bind(QHostAddress::Any,8080);
    // udpSocket->bind(QHostAddress("100.1.1.150"),8080);
    //  connect(udpSocket,SIGNAL(readyRead()),this,SLOT(read_data()));
    // QByteArray aaa;
    // aaa[0]=0xeb;
    // aaa[1]=0x90;
    // aaa[2]=0xa0;
    //aaa[3]=0x00;
    // aaa[4]=0x00;
    // aaa[5]=0x7b;

    // udpSocket->writeDatagram(aaa,aaa.size(),QHostAddress("100.1.1.187"),8080);

    /* QByteArray aaa;
    aaa[0]=0xeb;
    aaa[1]=0x90;
    aaa[2]=0xa0;
    aaa[3]=0x00;
    aaa[4]=0x00;
    aaa[5]=0x7b;
    emit datafile(aaa);*/
}

void MainWindow::on_pushButton_23_clicked()
{
    m_bySendBigCmd= 0x36;
    SendRemoteData();
    ui->statusBar->showMessage(tr("停止网络通道记录"));
}

void MainWindow::on_pushButton_24_clicked()
{

    //    QByteArray test;
    //    for(int i = 0; i < 24;i++)
    //    {
    //        test[i] = 0x00;
    //    }
    //    test[0] = 0xc7;
    //    test[1] =0xa7;
    //    test[3] =0x29;
    //    test[21] = 0x01;
    //    test[23]=0x98;

    m_bySendBigCmd= 0x55;
    SendRemoteData();
    ui->lineEdit->setText(selfdata.toHex(' ').toUpper());

    QMessageBox::StandardButton rb = QMessageBox::question(NULL, "警 告", "确定要物理自毁?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if(rb == QMessageBox::Yes)
    {
        m_bySendBigCmd= 0x56;
        SendRemoteData();

        ui->statusBar->showMessage(tr("物理自毁"));
    }
    //    my_serialport->write(test);
    qDebug()<<selfdata.toHex(' ').toUpper();
    ui->lineEdit->setText(selfdata.toHex(' ').toUpper());
}

void MainWindow::on_pushButton_10_clicked()
{

}

void MainWindow::on_radioButton_clicked()
{
    if(!bOpenFile)
    {
        ui->statusBar->showMessage(tr("文件未打开!"));
        return;
    }
    if(ui->radioButton->isChecked())
    {
        emit startWritingSignal();

        ui->radioButton->setText("关闭发送");
    }
    else
    {
        emit stopWritingSignal();
        ui->radioButton->setText("自动发送");
    }

}

void MainWindow::on_but_open_1_clicked()
{

    if(ui->comboBox__l->count() <= 0) return;

    f1=new Fcamera;
    connect(this,&MainWindow::cur_index,f1,&Fcamera::rev_slot);

    connect(f1,&Fcamera::sent_delete,this,&MainWindow::rev_delete);
    f1->show();
    ui->but_open_1->setEnabled(false);
    int a =ui->comboBox__l->currentIndex();
    emit cur_index(a);
}

void MainWindow::on_but_open_2_clicked()
{
    if(ui->comboBox__l->count() <= 0) return;

    f2 =new scamera;
    connect(this,&MainWindow::cur_index2,f2,&scamera::rev_slot);
    connect(f2,&scamera::sent_delete,this,&MainWindow::rev_delete2);
    f2->show();
    ui->but_open_2->setEnabled(false);
    int a =ui->comboBox__l->currentIndex();
    emit cur_index2(a);
}
void MainWindow::rev_delete()
{
    qDebug()<<"true";
    ui->but_open_1->setEnabled(true);
}
void MainWindow::rev_delete2()
{
    qDebug()<<"true";
    ui->but_open_2->setEnabled(true);
}


void MainWindow::on_pushButton_bc_clicked()
{
    //文件夹路径
    srcDirPath = QFileDialog::getExistingDirectory(
                this, "choose src Directory",
                "/");

    if (srcDirPath.isEmpty())
    {
        return;
    }
    else
    {

        srcDirPath += "/";
        qDebug() << "srcDirPath=" << srcDirPath;
        ui->label_bc->setText(srcDirPath);
        emit datafile(srcDirPath);
    }
}


void MainWindow::on_btn_DOON_clicked()
{
    QByteArray DOON;
    DOON.resize(8);
    DOON[0] = 0xfe;
    DOON[1] = 0x05;
    DOON[2] = 0x00;
    DOON[3] = 0x00;
    DOON[4] = 0xff;
    DOON[5] = 0x00;
    DOON[6] = 0x98;
    DOON[7] = 0x35;
    my_serialport3->write(DOON);
    qDebug()<<DOON.toHex(' ').toUpper();
    _sleep(50);
    DOON[3] = 0x01;
    DOON[6] = 0xc9;
    DOON[7] = 0xf5;
    my_serialport3->write(DOON);
    qDebug()<<DOON.toHex(' ').toUpper();
}

void MainWindow::on_btn_DOOFF_clicked()
{
    QByteArray DOOOF;
    DOOOF.resize(8);
    DOOOF[0] = 0xfe;
    DOOOF[1] = 0x05;
    DOOOF[2] = 0x00;
    DOOOF[3] = 0x00;
    DOOOF[4] = 0x00;
    DOOOF[5] = 0x00;
    DOOOF[6] = 0xD9;
    DOOOF[7] = 0xC5;
    my_serialport3->write(DOOOF);
    qDebug()<<DOOOF.toHex(' ').toUpper();
    _sleep(50);
    DOOOF[3] = 0x01;
    DOOOF[6] = 0x88;
    DOOOF[7] = 0x05;
    my_serialport3->write(DOOOF);
}

void MainWindow::on_btn_initcom_3_clicked()
{
    if(!IsStopCom3)
    {
        //关闭串口
        IsStopCom3 = true;
        ui->statusBar->showMessage(tr("停止%1串口通信").arg(ui->cbox_coms->currentText()),3000);
        ui->btn_initcom_3->setText(tr("打开串口"));
    }
    else
    {
        my_serialport3 = new Win_QextSerialPort(ui->cbox_coms_3->currentText(),QextSerialBase::EventDriven);

        if (my_serialport3->open(QIODevice::ReadWrite))
        {
            //设置波特率
            my_serialport3->setBaudRate(BAUD9600);

            //设置数据位
            my_serialport3->setDataBits(DATA_8);
            //设置校验位
            my_serialport3->setParity(PAR_NONE);
            //设置流控制
            my_serialport3->setFlowControl(FLOW_OFF);
            //设置停止位
            my_serialport3->setStopBits(STOP_1);
            ui->statusBar->showMessage(tr("%1初始化成功,串口通讯已开启").arg(ui->cbox_coms->currentText()),3000);
            ui->btn_initcom_3->setText(tr("关闭串口"));
        }
        else
        {
            QMessageBox::information(this, tr("提示"), tr("串口打开失败,请确认串口状态"), QMessageBox::Ok);
            return;
        }
        IsStopCom3 = false;
    }

}

void MainWindow::on_pushButton_open_timestamp_clicked()
{
    m_bySendBigCmd = 0xdd;
    SendRemoteData();
    ui->statusBar->showMessage(tr("打开视频时间戳叠加"));
}

void MainWindow::on_pushButton_close_timestam_clicked()
{
    m_bySendBigCmd = 0xdf;
    SendRemoteData();
    ui->statusBar->showMessage(tr("关闭视频时间戳叠加"));
}
