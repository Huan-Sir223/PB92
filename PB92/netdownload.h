#ifndef NETDOWNLOAD_H
#define NETDOWNLOAD_H

#include <QDialog>
#include <QtSql>

#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QDir>
#include <QDateTime>

#include <QList>
#include <QVector>

#define DATATYPE_SDI 10101
#define DATATYPE_422 10102
#define DATATYPE_CAMERLINK 10103
#define DATATYPE_PLOT 10104

#define SQLITE_IP "192.168.88.178"
//#define SQLITE_IP "192.168.99.100"
#define SQLITE_PORT 3002

#define DATADOWNLOAD_IP "192.168.88.178"
//#define DATADOWNLOAD_IP "192.168.99.100"
#define DATADOWNLOAD_PORT_SDI 10000
#define DATADOWNLOAD_PORT_CAM 9800

#define UDPSEND_PORT 9900

#define DOWNLOAD_CAM 10105
namespace Ui {
class NetDownload;
}

class NetDownload : public QDialog
{
    Q_OBJECT

public:
    explicit NetDownload(QWidget *parent = 0);
    ~NetDownload();

    void Init_all();

    int strHex2Int(QString str);//16进制字符串转int

    char* GetUdpcmd(int ilen, QString str,int icmd=4);

//    void tcpclient_disconnect();

//    void tcpdownload_disconnect();

private:
    Ui::NetDownload *ui;

    QTcpSocket *tcpClient;//用于连接数据库服务

    QTcpSocket *tcpDownLoad;//数据下载，SDI+422

    QTcpSocket *tcpDown_CAM;//数据下载，camerlink

    QUdpSocket *udpClient;//通过UDP发送下载camerlink数据指令

    int m_iDataType;//保存需要下载的数据类型

    QByteArray SQLbuffer;//保存sql服务的返回

    int m_iTabRowCount;//所有列表的行计数

    int m_iTabDownLoadRowCount;//下载列表的行计数

    QDir *dirDownLoad;

    QDateTime *dtDLoad;//保存下载的时间

    bool IsConnect_SQL;//数据库连接状态
    bool IsConnect_DownLoad;//数据下载连接状态

    bool IsFirst;//判断一个文件下载的第一接收数据
    bool IsOver;//判断一个文件下载是否结束

    QString CurDownFile;//当前正在下载的文件名

    QString CurFile_plot;//小区下载文件名解析
    QString NowFile;//当前小区文件
    qint64 iFileLen;//当前下载文件的总字节数
    qint64 iFileLen_Get;//当前文件已经接收的字节数

//    QList<QVector<QString>> CamerLink_dis;
    QVector<QString> str_disk;

    int disk_len;
    int disk_len_get;
    int disk_num;

    bool m_bIsUpdataTablist;

    QString FileFullPath;

    QDir *dir_dl;

public slots:
    void sel_sdi();
    void sel_422();
    void sel_camerlink();
    void sel_plot();

    void sel_CHN(QString);//

    void sel_FilePath();//修改文件下载路径

    void ReadData();//获取数据库服务返回

    void readData_CAM();//获取camerlink下载数据

    void Add_DownLoad();//添加下载
    void Delete_DownLoad();//删除下载

    void startDownLoad();//开始下载
    void recvDownLoad();

    void ReConnect();//网络重连
    void DialodClose();//退出按钮

    void slot_startDownload();//开始下载，槽函数
    void slot_download_CAM();

signals:
    void sgn_StartDownload();//信号，开始下载

    void sgn_download_CAM();//CAM下载
};

#endif // NETDOWNLOAD_H
