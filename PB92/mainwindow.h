#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QShortcut>
#include <QMessageBox>
#include <QKeySequence>

#include <QTcpServer>
#include <QTcpSocket>
#include <QFileDialog>
#include <QThread>

#include <QtSerialPort\qserialport.h>
#include <QtSerialPort\qserialportinfo.h>

#include <QTimer>
#include <QTreeWidgetItem>
#include "win_qextserialport.h"
#include "parsedata.h"
#include "netdownload.h"
#include "fcamera.h"
#include "scamera.h"
#include "felfouttime.h"
#include "tcpthread.h"
#include "mytcpserver.h"

//#define PB_57
//#define PB_60
#define PB_85

namespace Ui {
class MainWindow;
}
class udpTheread;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void GetAllCOMS();
    void delete_data();

    unsigned char * GetSysTime2Byte(unsigned char bytime[]);

    void InitParaName();

    int hex2int(QString strData,int len);

    bool checkbitValue(uchar,int );

    void Tree_AddItem(QTreeWidgetItem *_parent,QString _name,int &_color);
     bool bOpen;
     QByteArray arry;
     QTcpSocket *currentClient;


private:
    Ui::MainWindow *ui;

    MyTcpServer *tcpserver;
    QThread *thread;


    udpTheread* myworkThread;
    //实例化串口指针，用来对串口进行操作
//    QSerialPort * my_serialport;
    Win_QextSerialPort * my_serialport;//
    Win_QextSerialPort * my_serialport2;//
    Win_QextSerialPort * my_serialport3;//

    QTcpServer *tcpServer;
    //QList<QTcpSocket*> tcpClient;
    TcpThread *tcpthread = nullptr;


    bool bListen;
    bool bOpenFile;

    bool UseKey = false;

   // ParseData m_parseData;
   // QThread m_parseDataThread;

    NetDownload *Netdown;

    QTimer *Net_sendData;
    QTimer *TM_sendData;

    QTimer *RS_sendData;//飞参数据发送定时器

    QTimer *closeARM;//关闭ARM调试定时器

    QTimer *camera_s;//定时搜索摄像头设备

    FelfOutTime *felfout = nullptr;

    bool notself = true;

    bool IsTreeExpand;//是否展开treewidget,true:展开
    bool IsStopCom;//串口发送数据线程是否结束,true:结束
    bool IsStopCom2;//串口发送数据线程是否结束,true:结束
    bool IsStopCom3 = true;//串口发送数据线程是否结束,true:结束

    //0,启动/停止HD-SDI视频记录
    //1,启动/停止PAL模拟视频记录
    //2,启动Camera Link通道1记录
    //3,启动Camera Link通道2记录
    //4,启动同步422通道1记录
    //5,启动同步422通道2记录
    //6,启动同步422通道3记录
    //7,启动所有通道记录
    //8,启动HD-SDI视频小区记录
    //9,启动PAL模拟视频小区记录
    //10,启动Camera Link通道1小区记录
    //11,启动Camera Link通道2小区记录
    //12,启动同步422通道1小区记录
    //13,启动同步422通道2小区记录
    //14,启动同步422通道3小区记录
    //15,启动所有通道小区记录
    unsigned char m_byComcmd[16];
    unsigned char m_bySendBigCmd;
    unsigned char m_bySendSmallCmd;

    unsigned char m_bySendICR = 0x01;
    unsigned char m_bySendconstatus;

    int m_iFramCnt;//发送串口数据时的帧计数
    int m_iRecvFramCnt;

    QString g_strParaName[32];
    QString g_strArmParaNmae[50];

    int i_iaa55;
    int m_iContimes;

    bool m_bIsMyTime;//判断是否使用自定义时间

    bool m_sendRS422;//判断是否发送rs422

    int m_spSt;//串口状态计数
    int frame_num;//遥感帧计数

    unsigned short rs422_frame_num;//rs422帧计数

    bool CloseStatus = false;//1秒中状态

    QByteArray tempData;

    QByteArray selfdata;
     Fcamera *f1;
     scamera *f2;
    //QMenu *operaMenu;
    uchar startData;




    QUdpSocket *udpSocket;

    int i_b =0;

    int selfint = 5;
    int data_num;
    QString  srcDirPath;
    QString filePath;
    int filenum;
    QByteArray filedata;


    QString tile;
    int tile_num = 0;
    void StartData(int num);
    void StopData(int num);

signals:
     void cur_index(int str);
     void cur_index2(int str);
     void datafile(QString str);

    void startWritingSignal();//开始发送模拟网络数据
    void stopWritingSignal();//停止发送模拟网络数据
    void Disconnect();
public slots:
     void InEndtime();
     void handleShortcut_Z();
     void handleShortcut_X();
     void set_lisent(bool ok);

     void onNewConnection();

     void info_tcp_monitor(bool info);

    void rev_delete();
     void rev_delete2();
    void NewConnectionSlot();
    void InitCOMS();//初始化串口

    void GetComDta();//处理串口获取的数据

    void SendComDta();//发送数据
    void RSSendData();//飞参数据发送
    void NetSendData();//
    void SendRemoteData();//发送遥感数据

    void CloseARMStatus();//关闭ARM状态

    void SendARMcmd(int _len,unsigned char cmd);//ARM指令发送

    void On_stop_cmd();//停止发送命令

    void On_zhijian();//自检

    void camera_so();//搜索槽函数

    void read_data();//udp接受槽函数


    //大区
    void On_s_HDSDI();//开启HD-SDI通道1视频记录
    void On_s_HDSDI2();//开启HD-SDI通道2视频记录
    void On_s_HDSDI3();//开启HD-SDI通道3视频记录

    void On_s_PAL();//开启PAL
    void On_s_Link1();//开启camerlink通道1记录
    void On_s_Link2();
    void On_s_422_1();//开启422同步通道1记录
    void On_s_422_2();
    void On_s_422_3();
    void On_s_Asy422_1();//开启异步422通道1记录
    void On_s_Asy422_2();//开启异步422通道2记录
    void On_s_ALL();//开启所有通道记录

    void On_e_HDSDI();//停止HD-SDI视频记录
    void On_e_HDSDI2();//停止HD-SDI通道2视频记录
    void On_e_HDSDI3();//停止HD-SDI通道3视频记录
    void On_e_PAL();//停止PAL
    void On_e_Link1();//停止camerlink通道1记录
    void On_e_Link2();
    void On_e_422_1();//停止422同步通道1记录
    void On_e_422_2();
    void On_e_422_3();
    void On_e_Asy422_1();//停止异步422通道1记录
    void On_e_Asy422_2();//停止异步422通道2记录
    void On_e_ALL();//停止所有通道记录
    void On_DataDel();//数据销毁

    void On_Sdi1Set();//sdi-1编码参数设置
    void On_sdi2Set();//sdi-2编码参数设置
    void On_sdi3Set();//sdi-3编码参数设置
    void On_sdi4Set();//sdi-4编码参数设置
    void On_SendRS422();
    void On_Restoration();

    //小区
    void On_s_HDSDI_p();//开启HD-SDI视频记录
    void On_s_PAL_p();//开启PAL
    void On_s_Link1_p();//开启camerlink通道1记录
    void On_s_Link2_p();
    void On_s_422_1_p();//开启422同步通道1记录
    void On_s_422_2_p();
    void On_s_422_3_p();
    void On_s_ALL_p();//开启所有通道记录

    void On_e_HDSDI_p();//停止HD-SDI视频记录
    void On_e_PAL_p();//停止PAL
    void On_e_Link1_p();//停止camerlink通道1记录
    void On_e_Link2_p();
    void On_e_422_1_p();//停止422同步通道1记录
    void On_e_422_2_p();
    void On_e_422_3_p();
    void On_e_ALL_p();//停止所有通道记录

    //ARM
    void On_Tree_expand();//展开/合并Tree

    void On_connect();//请求连接
    void On_FileCmd();//文件列表请求
    void On_qDownLoad();//快速下载
    void On_ResetUnit();//复位存储单元
    void On_FormatStorage();//格式化存储
    void On_WriteRegister();//写数管板寄存器
    void On_ReadRegister();//ARM 关闭调试信息
    void On_PcieReset();
    void On_WriteSpeedTest(); //写速度测试

    void To_ParseData_AA55(QByteArray);
    void To_ParseData_AA55_20181228(QByteArray);
    void To_ParseData_B5A9(QByteArray);
    void To_ParseData_B5A9_20181228(QByteArray);
    void To_ParseData_B5A9_PB85(QByteArray);

    void To_ParseData_C7A7(QByteArray);
    void ToDo(QByteArray);

    void slot_rbtn_time();


    void On_s_data1();
    void On_s_data2();
    void On_s_data3();
    void On_s_data4();

    void On_e_data1();
    void On_e_data2();
    void On_e_data3();
    void On_e_data4();

    void On_s_net();
    void On_e_net();

signals:
    void ParseData_AA55(QByteArray);
    void ParseData_B5A9(QByteArray);
    void ParseData_C7A7(QByteArray);
    void Do(QByteArray);
    void StartTo();

private slots:
     void get_text();

    void on_pushButton_17_clicked();       
    void on_btn_initcom_2_clicked();
    void on_btn_stop_cmd_2_clicked();
    void on_pushButton_26_clicked();
    void on_pushButton_21_clicked();
    void on_pushButton_18_clicked();
    void on_pushButton_19_clicked();
    void on_pushButton_20_clicked();
    void on_pushButton_22_clicked();
    void on_pushButton_23_clicked();
    void on_pushButton_24_clicked();
    void on_pushButton_10_clicked();
    void on_radioButton_clicked();
    void on_but_open_1_clicked();
    void on_but_open_2_clicked();
    void on_pushButton_bc_clicked();
    void on_btn_DOON_clicked();
    void on_btn_DOOFF_clicked();
    void on_btn_initcom_3_clicked();
    void on_pushButton_open_timestamp_clicked();
    void on_pushButton_close_timestam_clicked();
};

class udpTheread:public QThread
{
    Q_OBJECT
protected:
    void run();
public:
    void delete_data();
    void get_data();
private:
    QFile file;

    QDateTime time;
    QByteArray dataarry;
    QString srcload;
    bool data_run ;


    QByteArray filedata;
     QByteArray filedata_2;
    int data_num;
     int i_b =0;
    QUdpSocket *udpSocket_t;
    int bzw;//0 11  1 22
    int z_num=0;
    int abc=0;
private slots:

     void get_load(QString str);


};
//MainWindow test;
#endif // MAINWINDOW_H
