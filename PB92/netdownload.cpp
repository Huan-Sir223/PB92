#include "netdownload.h"
#include "ui_netdownload.h"

#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <synchapi.h>
#include <QFileDialog>


NetDownload::NetDownload(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NetDownload)
{
    ui->setupUi(this);

    /*********************初始化表格设置************/
    ui->tableWidget_all->setColumnCount(4);

    ui->tableWidget_all->verticalHeader()->setDefaultSectionSize(25);

    QStringList headText;
    headText << "文件名"<<"通道号"<<"开始时间"<<"结束时间";
    ui->tableWidget_all->setHorizontalHeaderLabels(headText);
    ui->tableWidget_all->setSelectionBehavior(QAbstractItemView::SelectRows);//设置点击选择行
    ui->tableWidget_all->setEditTriggers(QAbstractItemView::NoEditTriggers);//设置不能编辑选择内容
//    ui->tableWidget_all->setSelectionMode(QAbstractItemView::SingleSelection);//设置只能选择一行，不能多行选中
    ui->tableWidget_all->setAlternatingRowColors(true);
    ui->tableWidget_all->verticalHeader()->setVisible(false);
    ui->tableWidget_all->horizontalHeader()->setStretchLastSection(true);

    ui->tableWidget_all->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_all->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableWidget_all->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableWidget_all->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableWidget_all->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    QHeaderView *head=ui->tableWidget_all->verticalHeader();
    head->setHidden(false);	//设置显示行号

    /////////////////////////////////////////////////
    ui->tableWidget_download->setColumnCount(5);

    ui->tableWidget_download->verticalHeader()->setDefaultSectionSize(25);

    QStringList headText_download;
    headText_download << "数据类型"<<"文件名"<<"通道号"<<"开始时间"<<"结束时间";
    ui->tableWidget_download->setHorizontalHeaderLabels(headText_download);
    ui->tableWidget_download->setSelectionBehavior(QAbstractItemView::SelectRows);//设置点击选择行
    ui->tableWidget_download->setEditTriggers(QAbstractItemView::NoEditTriggers);//设置不能编辑选择内容
    ui->tableWidget_download->setSelectionMode(QAbstractItemView::SingleSelection);//设置只能选择一行，不能多行选中
    ui->tableWidget_download->setAlternatingRowColors(true);
    ui->tableWidget_download->verticalHeader()->setVisible(false);
    ui->tableWidget_download->horizontalHeader()->setStretchLastSection(true);

    ui->tableWidget_download->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_download->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableWidget_download->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableWidget_download->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableWidget_download->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    QHeaderView *head_down=ui->tableWidget_download->verticalHeader();
    head_down->setHidden(false);

    /*********************************************************************/

    m_iDataType = 0;

    m_iTabRowCount = 1;

    m_iTabDownLoadRowCount = 1;

    IsFirst = false;
    IsOver = true;

    m_bIsUpdataTablist = true;

    ui->ledit_dir->setReadOnly(true);

    this->setWindowTitle("网络数据下载");

    Init_all();

    connect(ui->rbtn_SDI,SIGNAL(clicked(bool)),this,SLOT(sel_sdi()));
    connect(ui->rbtn_422,SIGNAL(clicked(bool)),this,SLOT(sel_422()));
    connect(ui->rbtn_link,SIGNAL(clicked(bool)),this,SLOT(sel_camerlink()));
    connect(ui->rbtn_xiaoqu,SIGNAL(clicked(bool)),this,SLOT(sel_plot()));

    connect(ui->cbox_chn,SIGNAL(currentTextChanged(QString)),this,SLOT(sel_CHN(QString)));

    connect(ui->btn_add,SIGNAL(clicked(bool)),this,SLOT(Add_DownLoad()));
    connect(ui->btn_download,SIGNAL(clicked(bool)),this,SLOT(startDownLoad()));
//    connect(ui->tableWidget_all,SIGNAL(clicked(QModelIndex)))
    connect(ui->btn_del,SIGNAL(clicked(bool)),this,SLOT(Delete_DownLoad()));
    connect(ui->btn_exit,SIGNAL(clicked(bool)),this,SLOT(DialodClose()));

    connect(ui->btn_dir,SIGNAL(clicked(bool)),this,SLOT(sel_FilePath()));

    connect(this,SIGNAL(sgn_StartDownload()),this,SLOT(slot_startDownload()));

    connect(this,SIGNAL(sgn_download_CAM()),this,SLOT(slot_download_CAM()));
}

NetDownload::~NetDownload()
{
    delete ui;
}

void NetDownload::Init_all()
{
    dirDownLoad = new QDir("D:/RecorderDowmLoad");
    if(!dirDownLoad->exists())
    {
        dirDownLoad->mkdir("D:/RecorderDowmLoad");
    }

    ui->ledit_dir->setText(dirDownLoad->absolutePath());

    //************连接数据库服务的TCP服务器
    tcpClient = new QTcpSocket(this);
    tcpClient->abort();
    tcpClient->connectToHost(SQLITE_IP,SQLITE_PORT);

    if (tcpClient->waitForConnected(3000))  // 连接成功则进入if{}
    {
        //测试SQL服务是否正常
        tcpClient->write(":PPRAGMA VER\r");

        tcpClient->write(":PPRAGMA ETX\r");
        IsConnect_SQL = true;
        connect(this->tcpClient, SIGNAL(readyRead()), this, SLOT(ReadData()));
    }
    else
    {
        IsConnect_SQL = false;
        QMessageBox::about(this,"提示","连接SQL服务器失败");
        this->close();
    }

    //************连接数据下载TCP服务器
    tcpDownLoad = new QTcpSocket(this);
    tcpDownLoad->abort();

    tcpDownLoad->connectToHost(DATADOWNLOAD_IP,DATADOWNLOAD_PORT_SDI);

    if(!tcpDownLoad->waitForConnected(3000))
    {
        QMessageBox::about(this,"提示","连接DOWN服务器失败");
        IsConnect_DownLoad = false;
        return;
    }
    else
    {
        connect(this->tcpDownLoad,SIGNAL(readyRead()),this,SLOT(recvDownLoad()));
        IsConnect_DownLoad = true;
    }

    tcpDown_CAM = new QTcpSocket(this);
    tcpDown_CAM->abort();

    tcpDown_CAM->connectToHost(DATADOWNLOAD_IP,DATADOWNLOAD_PORT_CAM);
    if(!tcpDown_CAM->waitForConnected(3000))
    {
        QMessageBox::about(this,"提示","连接服务器失败");
        return;
    }
    else
    {
        connect(this->tcpDown_CAM,SIGNAL(readyRead()),this,SLOT(readData_CAM()));
    }

    udpClient = new QUdpSocket(this);
}

void NetDownload::sel_sdi()
{
    m_iDataType = DATATYPE_SDI;
    ui->cbox_chn->clear();
    ui->cbox_chn->insertItem(0,"all");
    ui->cbox_chn->insertItem(1,"0");
    ui->cbox_chn->insertItem(2,"1");
}

void NetDownload::sel_422()
{
    m_iDataType = DATATYPE_422;
    ui->cbox_chn->clear();
    ui->cbox_chn->insertItem(0,"all");
    ui->cbox_chn->insertItem(1,"1");
    ui->cbox_chn->insertItem(2,"2");
    ui->cbox_chn->insertItem(3,"3");
}

void NetDownload::sel_camerlink()
{
    m_iDataType = DATATYPE_CAMERLINK;
    ui->cbox_chn->clear();
    ui->cbox_chn->insertItem(0,"all");
    ui->cbox_chn->insertItem(1,"1");
    ui->cbox_chn->insertItem(2,"2");
}

void NetDownload::sel_plot()
{
    m_iDataType = DATATYPE_PLOT;
    ui->cbox_chn->clear();
}


void NetDownload::sel_CHN(QString strCHN)
{
    if(strCHN == "")return;
    SQLbuffer.clear();
    m_iTabRowCount = 1;

    ui->tableWidget_all->setRowCount(m_iTabRowCount-1);

    if(m_iDataType == DATATYPE_SDI)
    {
        if(strCHN == "all")
        {
            QByteArray btSql = "select FNAME,CHN,STIME,ETIME from STORAGE where TYPE = 1;";
            btSql.append((char)3);

            tcpClient->write(btSql);
        }
        else if(strCHN == "0")
        {
            QByteArray btSql = "select FNAME,CHN,STIME,ETIME from STORAGE where TYPE = 1 AND CHN = 0;";
            btSql.append((char)3);

            tcpClient->write(btSql);
        }
        else if(strCHN == "1")
        {
            QByteArray btSql = "select FNAME,CHN,STIME,ETIME from STORAGE where TYPE = 1 AND CHN = 1;";
            btSql.append((char)3);

            tcpClient->write(btSql);
        }
        else
        {

        }

    }
    else if(m_iDataType == DATATYPE_422)
    {
        if(strCHN == "all")
        {
            QByteArray btSql = "select FNAME,CHN,STIME,ETIME from STORAGE where TYPE = 4;";
            btSql.append((char)3);

            tcpClient->write(btSql);
        }
        else if(strCHN == "1")
        {
            QByteArray btSql = "select FNAME,CHN,STIME,ETIME from STORAGE where TYPE = 4 AND CHN = 1;";
            btSql.append((char)3);

            tcpClient->write(btSql);
        }
        else if(strCHN == "2")
        {
            QByteArray btSql = "select FNAME,CHN,STIME,ETIME from STORAGE where TYPE = 4 AND CHN = 2;";
            btSql.append((char)3);

            tcpClient->write(btSql);
        }
        else if(strCHN == "3")
        {
            QByteArray btSql = "select FNAME,CHN,STIME,ETIME from STORAGE where TYPE = 4 AND CHN = 3;";
            btSql.append((char)3);

            tcpClient->write(btSql);
        }
        else
        {

        }
    }
    else if(m_iDataType == DATATYPE_CAMERLINK)
    {
        if(strCHN == "all")
        {
            QByteArray btSql_1 = "select LBA,CHN,DISKID,TIME from CLHEADER";
            btSql_1.append((char)3);

            tcpClient->write(btSql_1);
        }
        else if(strCHN == "1")
        {
            QByteArray btSql = "select LBA,CHN,DISKID,TIME from CLHEADER where CHN = 1;";
            btSql.append((char)3);

            tcpClient->write(btSql);
        }
        else if(strCHN == "2")
        {
            QByteArray btSql = "select LBA,CHN,DISKID,TIME from CLHEADER where CHN = 2;";
            btSql.append((char)3);

            tcpClient->write(btSql);
        }

    }
    else if(m_iDataType == DATATYPE_PLOT)
    {

    }
    else
    {

    }

}

void NetDownload::startDownLoad()
{
    if(!IsConnect_DownLoad)
    {
        QMessageBox::about(this,"提示","网络状态未连接，请尝试重新连接网络");
        return ;
    }
//    QString FileFullPath;

//    QDir *dir_dl = new QDir(ui->ledit_dir->text());
    dir_dl = new QDir(ui->ledit_dir->text());

    if(!dir_dl->exists())
    {
        dir_dl->mkdir(ui->ledit_dir->text());
        QDateTime *nowtime = new QDateTime(QDateTime::currentDateTime());
        FileFullPath = ui->ledit_dir->text() + "/" + nowtime->toString("yyyy-MM-dd-hh-mm-ss");

        dir_dl->mkdir(FileFullPath);
    }
    else
    {
        QDateTime *nowtime = new QDateTime(QDateTime::currentDateTime());
        FileFullPath = ui->ledit_dir->text() + "/" + nowtime->toString("yyyy-MM-dd-hh-mm-ss");

        dir_dl->mkdir(FileFullPath);
    }

    if(m_iDataType == DATATYPE_PLOT)
    {
        CurDownFile = FileFullPath + "/" + "小区数据";
        dir_dl = new QDir(CurDownFile);
        if(!dir_dl->exists())
        {
            dir_dl->mkdir(CurDownFile);
        }

        IsFirst = true;
        IsOver = false;
        tcpDownLoad->write("downloadL");

        return ;
    }
    else
    {
        emit sgn_StartDownload();
    }

    /*
    int dl_Count = m_iTabDownLoadRowCount-1;

    for(int i = dl_Count-1; i >= 0 ; i--)
    {
        if(IsOver)
        {
            QString strType = ui->tableWidget_download->item(i,0)->text();
            QString strFileName = ui->tableWidget_download->item(i,1)->text();
            QString strSTime = ui->tableWidget_download->item(i,3)->text();
            QString strETime = ui->tableWidget_download->item(i,4)->text();

            ui->tableWidget_download->removeRow(i+1);

            CurDownFile = FileFullPath + "/" + strType;
            dir_dl = new QDir(CurDownFile);
            if(!dir_dl->exists())
            {
                dir_dl->mkdir(CurDownFile);
            }

            CurDownFile = CurDownFile + "/" + strFileName;

            QFile mfile(CurDownFile);
            mfile.open(QFile::ReadWrite);
            mfile.close();

            iFileLen_Get = 0;

            if(strType == "CamerLink")
            {
                QHostAddress _addr(DATADOWNLOAD_IP);

//                tcpDown_CAM = new QTcpSocket(this);
//                tcpDown_CAM->abort();

//                tcpDown_CAM->connectToHost(DATADOWNLOAD_IP,DATADOWNLOAD_PORT_CAM);
//                if(!tcpDown_CAM->waitForConnected(3000))
//                {
//                    QMessageBox::about(this,"提示","连接服务器失败");
//                    return;
//                }
//                else
//                {
////                    connect(this->tcpDown_CAM,SIGNAL(readyRead()),this,SLOT(readData_CAM()));
//                }

                QString strsql = QString("select LBA,CHN,DISKID,TIME from CLHEADER WHERE (TIME BETWEEN '%1' AND '%2');").arg(strSTime).arg(strETime);

                QByteArray btSql_2 = strsql.toUtf8();
                btSql_2.append((char)3);

                IsOver = false;

                SQLbuffer.clear();
                SQLbuffer.resize(0);

                str_disk.clear();
                QVector<QString> pNullVector;

                str_disk.swap(pNullVector);

                qDebug()<<btSql_2;
                m_bIsUpdataTablist = false;
                tcpClient->write(btSql_2);

                while (1)
                {
                    if(IsOver)
                    {
                        m_bIsUpdataTablist = true;
                        disk_len = str_disk.length();
                        disk_num = 0;
                        for(int j = 0;j< str_disk.length();j++)
                        {
                            if(IsOver)
                            {
                                QString dis = str_disk.at(j);

                                char* chTest = GetUdpcmd(8,dis);

                                IsOver = false;

                                disk_len_get = 0;

                                udpClient->writeDatagram(chTest,8,_addr,UDPSEND_PORT);
                            }
                            else
                            {
                                j--;
                                qApp->processEvents();
                            }
                        }

                        char* chTest = GetUdpcmd(8,"",3);

                        udpClient->writeDatagram(chTest,8,_addr,UDPSEND_PORT);

                        qDebug()<<"over";

                        break;
                    }
                    else
                    {
                        qApp->processEvents();
                    }
                }


            }
            else
            {
                tcpDownLoad->write(strFileName.toUtf8());
            }

//            disconnect(this->tcpDown_CAM,SIGNAL(readyRead()),this,SLOT(readData_CAM()));
            IsOver = false;
            IsFirst = true;
        }
        else
        {
            qApp->processEvents();
            i++;
        }
    }
    ui->tableWidget_download->removeRow(0);

    m_iTabDownLoadRowCount = 1;
    */
}


void NetDownload::ReadData()//获取查询数据库数据项的返回
{
    SQLbuffer.append(tcpClient->readAll());

    if(SQLbuffer.indexOf(":OK") > 0 && SQLbuffer.indexOf(":R") > 0)//":OK" 是返回结束的标识，":R"是查询到的结果集
    {
        m_iTabRowCount = 1;

        QString str_one;
        QString str_two;
        QString str_three;
        QString str_four;

        QString str_sTime;
        QString str_eTime;
        int iStart = SQLbuffer.indexOf(":R")+3;
        int iEnd = SQLbuffer.indexOf(":OK");
        QByteArray usedArray = SQLbuffer.mid(iStart,iEnd-iStart);

        int i = 0;

        while (++i)
        {
            //解文件名  one
            iStart = 0;
            iEnd = usedArray.indexOf((char)3);
            if(iEnd <= 0 && i%256 != 0 && m_iDataType == DATATYPE_CAMERLINK && m_bIsUpdataTablist)
            {
                QString strFileName;
                strFileName.sprintf("%06d",m_iTabRowCount);

                ui->tableWidget_all->setRowCount(m_iTabRowCount);
                ui->tableWidget_all->setRowHeight(m_iTabRowCount-1,24);

                QTableWidgetItem * itemFileName = new QTableWidgetItem(strFileName);
                QTableWidgetItem * itemCHN = new QTableWidgetItem(str_two);
                QTableWidgetItem * itemSTime = new QTableWidgetItem(str_sTime);
                QTableWidgetItem * itemETime = new QTableWidgetItem(str_eTime);

                ui->tableWidget_all->setItem(m_iTabRowCount-1,0,itemFileName);
                ui->tableWidget_all->setItem(m_iTabRowCount-1,1,itemCHN);
                ui->tableWidget_all->setItem(m_iTabRowCount-1,2,itemSTime);
                ui->tableWidget_all->setItem(m_iTabRowCount-1,3,itemETime);

                m_iTabRowCount++;
                break;
            }
            else if(iEnd <= 0 && i%256 == 0)
            {
                break;
            }

            str_one = usedArray.left(iEnd);
            usedArray = usedArray.mid(iEnd+1);

            //解通道号  two
            iStart = 0;
            iEnd = usedArray.indexOf((char)3);
            if(iEnd <= 0)
            {
                break;
            }

            str_two = usedArray.left(iEnd);
            usedArray = usedArray.mid(iEnd+1);

            //解Stime  three
            iStart = 0;
            iEnd = usedArray.indexOf((char)3);
            if(iEnd <= 0)
            {
                break;
            }

            str_three = usedArray.left(iEnd);
            usedArray = usedArray.mid(iEnd+1);

            //解Etime   four
            iStart = 0;
            iEnd = usedArray.indexOf((char)3);
            if(iEnd <= 0)
            {
                break;
            }

            str_four = usedArray.left(iEnd);
            usedArray = usedArray.mid(iEnd+1);

            if(!m_bIsUpdataTablist)
            {
                str_disk.append(str_one+str_three);
            }
            else if(m_iDataType == DATATYPE_SDI || m_iDataType == DATATYPE_422)
            {
                ui->tableWidget_all->setRowCount(m_iTabRowCount);
                ui->tableWidget_all->setRowHeight(m_iTabRowCount-1,24);

                QTableWidgetItem * itemFileName = new QTableWidgetItem(str_one);
                QTableWidgetItem * itemCHN = new QTableWidgetItem(str_two);
                QTableWidgetItem * itemSTime = new QTableWidgetItem(str_three);
                QTableWidgetItem * itemETime = new QTableWidgetItem(str_four);

                ui->tableWidget_all->setItem(m_iTabRowCount-1,0,itemFileName);
                ui->tableWidget_all->setItem(m_iTabRowCount-1,1,itemCHN);
                ui->tableWidget_all->setItem(m_iTabRowCount-1,2,itemSTime);
                ui->tableWidget_all->setItem(m_iTabRowCount-1,3,itemETime);

                m_iTabRowCount++;
            }
            else if(m_iDataType == DATATYPE_CAMERLINK)
            {
                if(i%256 == 1)
                    str_sTime = str_four;
                else// if(i%256 == 0)
                {
                    str_eTime = str_four;
                }

                if(i%256 == 0)
                {
                    QString strFileName;
                    strFileName.sprintf("%06d",m_iTabRowCount);

                    ui->tableWidget_all->setRowCount(m_iTabRowCount);
                    ui->tableWidget_all->setRowHeight(m_iTabRowCount-1,24);

                    QTableWidgetItem * itemFileName = new QTableWidgetItem(strFileName);
                    QTableWidgetItem * itemCHN = new QTableWidgetItem(str_two);
                    QTableWidgetItem * itemSTime = new QTableWidgetItem(str_sTime);
                    QTableWidgetItem * itemETime = new QTableWidgetItem(str_eTime);

                    ui->tableWidget_all->setItem(m_iTabRowCount-1,0,itemFileName);
                    ui->tableWidget_all->setItem(m_iTabRowCount-1,1,itemCHN);
                    ui->tableWidget_all->setItem(m_iTabRowCount-1,2,itemSTime);
                    ui->tableWidget_all->setItem(m_iTabRowCount-1,3,itemETime);

                    m_iTabRowCount++;
                }
            }

        }

        if(!m_bIsUpdataTablist)
        {
            disk_len = str_disk.length();
            emit sgn_download_CAM();
        }
//        IsOver = true;

    }
}


void NetDownload::Add_DownLoad()
{
    QList<QTableWidgetItem*> items=ui->tableWidget_all->selectedItems();

    int count=items.count();

    int o_count=m_iTabDownLoadRowCount;

    for(int i=0;i<count/4;i++)
    {
        if(0)
        {
number1:
            continue;
        }

        QTableWidgetItem* item=items.at(4*i);

        QString strFileName=item->text();//获取内容

        while (o_count > 1) {
            QString o_name = ui->tableWidget_download->item(o_count-2,1)->text();
            if(o_name == strFileName)
            {
                QMessageBox::about(this,"提示","该数据已经在下载列表，无需再次添加");
                goto number1;
            }
            o_count--;
        }

        QString DataType;
        if(m_iDataType == DATATYPE_SDI)
        {
            DataType = "SDI";
        }
        else if(m_iDataType == DATATYPE_422)
        {
            DataType = "同步422";
        }
        else if(m_iDataType == DATATYPE_CAMERLINK || m_iDataType == DOWNLOAD_CAM)
        {
            DataType = "CamerLink";
        }

        item = items.at(4*i+1);

        QString strCHN = item->text();

        item = items.at(4*i+2);

        QString strSTime = item->text();

        item = items.at(4*i+3);

        QString strETime = item->text();

        QTableWidgetItem * itemDataType = new QTableWidgetItem(DataType);
        QTableWidgetItem * itemFileName = new QTableWidgetItem(strFileName);
        QTableWidgetItem * itemCHN = new QTableWidgetItem(strCHN);
        QTableWidgetItem * itemSTime = new QTableWidgetItem(strSTime);
        QTableWidgetItem * itemETime = new QTableWidgetItem(strETime);

        ui->tableWidget_download->setRowCount(m_iTabDownLoadRowCount);
        ui->tableWidget_download->setRowHeight(m_iTabDownLoadRowCount-1,24);

        ui->tableWidget_download->setItem(m_iTabDownLoadRowCount-1,0,itemDataType);
        ui->tableWidget_download->setItem(m_iTabDownLoadRowCount-1,1,itemFileName);
        ui->tableWidget_download->setItem(m_iTabDownLoadRowCount-1,2,itemCHN);
        ui->tableWidget_download->setItem(m_iTabDownLoadRowCount-1,3,itemSTime);
        ui->tableWidget_download->setItem(m_iTabDownLoadRowCount-1,4,itemETime);

        m_iTabDownLoadRowCount++;
    }
}

void NetDownload::recvDownLoad()
{
    //下载小区数据（SDI + 同步422）
    if(m_iDataType == DATATYPE_PLOT)
    {
        QByteArray File_Value;

        if(IsFirst)
        {
            qDebug()<<"first";
            iFileLen_Get = 0;
            if(tcpDownLoad->bytesAvailable() < 36)
                return;
            IsFirst = false;
            QByteArray File_len = tcpDownLoad->read(4);

            iFileLen = ((UCHAR)File_len.at(0)) + ((UCHAR)File_len.at(1)) * pow(16.0,2.0) + ((UCHAR)File_len.at(2)) * pow(16.0,4.0) + ((UCHAR)File_len.at(3)) * pow(16.0,6.0);

            qDebug()<<"文件长度"<<iFileLen;

            CurFile_plot = tcpDownLoad->read(32);

            NowFile = CurDownFile + "/" + CurFile_plot;

            qDebug()<<"文件名称"<<NowFile;

            QFile mfile(NowFile);
            mfile.open(QFile::ReadWrite);
            mfile.close();
        }

//        if(tcpDownLoad->bytesAvailable() + iFileLen_Get > iFileLen)
//        {
//            File_Value = tcpDownLoad->read(iFileLen-iFileLen_Get);
//        }
//        else
//        {
//            File_Value = tcpDownLoad->readAll();
//        }
        File_Value = tcpDownLoad->readAll();

        iFileLen_Get += File_Value.length();

        qDebug()<<"已接收"<<iFileLen_Get;

        ui->proBar->setValue((iFileLen_Get*100)/iFileLen);

        QFile m_file(NowFile);
        m_file.open(QIODevice::WriteOnly|QIODevice::Append);

        m_file.write(File_Value);
        m_file.close();
        if(iFileLen_Get >= iFileLen)
        {
            qDebug()<<"one File";
            IsFirst = true;
            IsOver = true;
            iFileLen = 0;
            ui->proBar->setValue(100);

            int k = tcpDownLoad->write("NEXT",5);
            qDebug()<<"next"<<k;
            tcpDownLoad->flush();
        }

        return;
    }

    //获取SDI,422数据
    if(IsFirst)
    {
        IsFirst = false;
        QByteArray File_len = tcpDownLoad->read(4);

        iFileLen = ((UCHAR)File_len.at(0)) + ((UCHAR)File_len.at(1)) * pow(16.0,2.0) + ((UCHAR)File_len.at(2)) * pow(16.0,4.0) + ((UCHAR)File_len.at(3)) * pow(16.0,6.0);
    }

    QByteArray File_Value = tcpDownLoad->readAll();

    iFileLen_Get += File_Value.length();

    ui->proBar->setValue((iFileLen_Get*100)/iFileLen);

    QFile m_file(CurDownFile);
    m_file.open(QIODevice::WriteOnly|QIODevice::Append);

    m_file.write(File_Value);
    m_file.close();
    if(iFileLen_Get == iFileLen)
    {
        iFileLen_Get = 0;
        iFileLen = 0;
        ui->proBar->setValue(100);

        emit sgn_StartDownload();
    }
}

void NetDownload::ReConnect()
{
    tcpClient->disconnectFromHost();
    tcpDownLoad->disconnectFromHost();
    //************连接数据库服务的TCP服务器
    tcpClient = new QTcpSocket(this);
    tcpClient->abort();
    tcpClient->connectToHost(SQLITE_IP,SQLITE_PORT);

    if (tcpClient->waitForConnected(3000))  // 连接成功则进入if{}
    {
        //测试SQL服务是否正常
        tcpClient->write(":PPRAGMA VER\r");

        tcpClient->write(":PPRAGMA ETX\r");

        QMessageBox::about(this,"提示","连接成功");

        IsConnect_SQL = true;
    }
    else
    {
        IsConnect_SQL = false;
        QMessageBox::about(this,"提示","连接失败");
    }

    //************连接数据下载TCP服务器
    tcpDownLoad = new QTcpSocket(this);
    tcpDownLoad->abort();

    tcpDownLoad->connectToHost(DATADOWNLOAD_IP,DATADOWNLOAD_PORT_SDI);

    if(!tcpDownLoad->waitForConnected(3000))
    {
        QMessageBox::about(this,"提示","连接服务器失败");
        IsConnect_DownLoad = false;
        return;
    }
    else
    {
        QMessageBox::about(this,"提示","连接成功");

        IsConnect_DownLoad = true;
    }
}

void NetDownload::Delete_DownLoad()
{
    if(m_iTabDownLoadRowCount <= 1)
    {
        m_iTabDownLoadRowCount = 1;
        return;
    }
    ui->tableWidget_download->removeRow(ui->tableWidget_download->currentRow());

    m_iTabDownLoadRowCount--;
}

void NetDownload::DialodClose()
{
    qDebug()<<"close";
//    tcpClient->abort();
    tcpClient->close();

//    tcpDownLoad->abort();
    tcpDownLoad->close();

    qDebug()<<"close";
    QHostAddress _addr(DATADOWNLOAD_IP);
    char* chTest = GetUdpcmd(8,"",3);

    qDebug()<<"close";
    udpClient->writeDatagram(chTest,8,_addr,UDPSEND_PORT);
    qDebug()<<"close";

//    tcpDown_CAM->abort();
    tcpDown_CAM->close();

    qDebug()<<"close";
    this->close();
}

int NetDownload::strHex2Int(QString str)
{
    int iRet = 0, itemp = 0;
        for (int i = 0; i < str.length(); i++) {
            if(str.toStdString()[i] >= 'A'&& str.toStdString()[i] <= 'F') {
                itemp = str.toStdString()[i] - 'A' + 10;
            } else if(str.toStdString()[i] >= 'a' && str.toStdString()[i] <= 'f') {
                itemp = str.toStdString()[i] - 'a' + 10;
            } else {
                itemp = str.toStdString()[i] - '0';
            }
            iRet = iRet * 16 + itemp;
        }
        return iRet;
}

char* NetDownload::GetUdpcmd(int ilen, QString str, int icmd)
{
    str.replace(" ","");
    char* ch;
    if (ilen < 0) {
        return NULL;
    } else {
        ch = new char[ilen];
    }

    int itemp = atoi(str.left(str.length() - 1).toStdString().c_str());
    QString strCmd = "";
    char chcmd[10] = {0};
    itoa(itemp, chcmd, 16);
    strCmd = chcmd;
    int iFlag = strCmd.length();
    for (int i = 0; i < 12 - iFlag; i++) {
        strCmd = "0" + strCmd;
    }

    for (int i = 0; i < 6; i++) {
        ch[i + 1] = strHex2Int(strCmd.mid(i * 2, 2));
    }

    ch[0] = icmd;
    ch[7] = atoi(str.mid(str.length() - 1, 1).toStdString().c_str());

    return ch;
}

void NetDownload::readData_CAM()
{
    //获取Camerlink，下载的数据
    QByteArray File_Value = tcpDown_CAM->readAll();

    disk_len_get += File_Value.length();

    QFile m_file(CurDownFile);
    if(!m_file.open(QIODevice::WriteOnly|QIODevice::Append))
    {
        qDebug()<<"not open";
        return;
    }

    m_file.write(File_Value);
    m_file.close();

    if(disk_len_get == 4*1024*1024)
    {
        disk_num++;
        ui->proBar->setValue((disk_num*100)/disk_len);
        emit sgn_download_CAM();
    }

    return;
}

void NetDownload::sel_FilePath()
{
    QString _filepath = QFileDialog::getExistingDirectory(this,"选择文件夹","./");

    ui->ledit_dir->setText(_filepath);
}


void NetDownload::slot_startDownload()
{
//    QString FileFullPath;

//    QDir *dir_dl = new QDir(ui->ledit_dir->text());

//    if(!dir_dl->exists())
//    {
//        dir_dl->mkdir(ui->ledit_dir->text());
//        QDateTime *nowtime = new QDateTime(QDateTime::currentDateTime());
//        FileFullPath = ui->ledit_dir->text() + "/" + nowtime->toString("yyyy-MM-dd-hh-mm-ss");

//        dir_dl->mkdir(FileFullPath);
//    }
//    else
//    {
//        QDateTime *nowtime = new QDateTime(QDateTime::currentDateTime());
//        FileFullPath = ui->ledit_dir->text() + "/" + nowtime->toString("yyyy-MM-dd-hh-mm-ss");

//        dir_dl->mkdir(FileFullPath);
//    }

    int dl_Count = m_iTabDownLoadRowCount-1;

    if(dl_Count == 0)
        return ;

    //获取当前下载行的数据值
    QString strType = ui->tableWidget_download->item(0,0)->text();
    QString strFileName = ui->tableWidget_download->item(0,1)->text();
    QString strSTime = ui->tableWidget_download->item(0,3)->text();
    QString strETime = ui->tableWidget_download->item(0,4)->text();

    //创建本地保存文件
    CurDownFile = FileFullPath + "/" + strType;
    dir_dl = new QDir(CurDownFile);
    if(!dir_dl->exists())
    {
        dir_dl->mkdir(CurDownFile);
    }

    CurDownFile = CurDownFile + "/" + strFileName;

    QFile mfile(CurDownFile);
    mfile.open(QFile::ReadWrite);
    mfile.close();

    iFileLen_Get = 0;

    IsFirst = true;

    if(strType == "CamerLink")
    {


//                tcpDown_CAM = new QTcpSocket(this);
//                tcpDown_CAM->abort();

//                tcpDown_CAM->connectToHost(DATADOWNLOAD_IP,DATADOWNLOAD_PORT_CAM);
//                if(!tcpDown_CAM->waitForConnected(3000))
//                {
//                    QMessageBox::about(this,"提示","连接服务器失败");
//                    return;
//                }
//                else
//                {
////                    connect(this->tcpDown_CAM,SIGNAL(readyRead()),this,SLOT(readData_CAM()));
//                }

        QString strsql = QString("select LBA,CHN,DISKID,TIME from CLHEADER WHERE (TIME BETWEEN '%1' AND '%2');").arg(strSTime).arg(strETime);

        QByteArray btSql_2 = strsql.toUtf8();
        btSql_2.append((char)3);

//        IsOver = false;

        SQLbuffer.clear();
        SQLbuffer.resize(0);

        str_disk.clear();
        QVector<QString> pNullVector;

        str_disk.swap(pNullVector);

//        qDebug()<<btSql_2;
        m_bIsUpdataTablist = false;
        tcpClient->write(btSql_2);

        disk_num = 0;

    }
    else if(strType == "SDI" || strType == "同步422")
    {
        tcpDownLoad->write(strFileName.toUtf8());
    }
    else
    {
        QMessageBox::about(this,"About","软件异常，请重启软件/r/n  --详细信息：数据类型错误");
    }

    ui->tableWidget_download->removeRow(0);
    m_iTabDownLoadRowCount -= 1;
}

void NetDownload::slot_download_CAM()
{
    QHostAddress _addr(DATADOWNLOAD_IP);

    if(disk_num == disk_len)
    {
        //完成一次下载
        m_bIsUpdataTablist = true;
        emit sgn_StartDownload();
        return ;
    }

    QString dis = str_disk.at(disk_num);

    char* chTest = GetUdpcmd(8,dis);

    disk_len_get = 0;

    udpClient->writeDatagram(chTest,8,_addr,UDPSEND_PORT);
}



