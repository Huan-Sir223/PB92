#include "fcamera.h"
#include "ui_fcamera.h"
#include <qdebug.h>

Fcamera::Fcamera(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Fcamera)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_QuitOnClose, false);
    this->setAttribute(Qt::WA_DeleteOnClose);

    m_cap = new VideoCapture();
    connect(this, &Fcamera::updateImage, ui->widget, &PlayImage::updateImage);

}

Fcamera::~Fcamera()
{
     qDebug()<<"delete1";
    if(m_cap)
    {
        if(m_cap->isOpened())
        {
            quitThread();
        }
         qDebug()<<"delete2";
        delete m_cap;
    }
    delete ui;
    qDebug()<<"delete3";
    emit sent_delete();
}
void Fcamera::readImage()
{
    double fps = m_cap->get(CAP_PROP_FPS);   // 获取帧率，有时候获取不到帧率，如摄像头，就使用默认帧率25
    qDebug()<<"fps="<<fps;
    if(fps < 1)
    {
        fps = 25;
    }
   // writer.open("./1.avi", VideoWriter::fourcc('M','J','P','G'), fps, s, true);   // 打开保存视频文件，帧率25

    while (m_play && m_cap->isOpened())
    {
        bool ret = m_cap->read(mat);
        if(ret)
        {

            emit this->updateImage(MatToQImage(mat));   // 将mat转换为Qimage并发送给显示界面
        }
        else
        {
            QThread::msleep(1);           // 防止频繁读取或者读取不到
        }

    }
    m_play = true;
}
void Fcamera::quitThread()
{
    m_play = false;
    while (!m_play) {
        QThread::msleep(1);           // 等待线程退出
    }
    m_play = false;
}
void Fcamera::rev_slot(int str)
{
    qDebug()<<str;
    qDebug()<<"1";
    if(!m_cap->isOpened())
    {
        qDebug()<<"2";
       // if(ui->comboBox->count() <= 0) return;

        bool ret = m_cap->open(str);
        if(ret)
        {

            m_play = true;
           // ui->but_open->setText("关闭摄像头");
            QtConcurrent::run(this, &Fcamera::readImage);   // 在线程中读取
        }
    }
    else
    {
        qDebug()<<"3";
       quitThread();
        m_cap->release();
       // ui->but_open->setText("打开摄像头");
        //nu=1;
    }
}
QImage Fcamera::MatToQImage(const Mat &mat)
{
   // qDebug()<<"mat.type()";
    switch (mat.type())
    {

    case CV_8UC1:
        {
            QImage img(mat.data, mat.cols, mat.rows, mat.cols, QImage::Format_Grayscale8);
            return img;
        }
    case CV_8UC3:
        {
            QImage img(mat.data, mat.cols, mat.rows, mat.cols * 3, QImage::Format_RGB888);
            return img.rgbSwapped();
        }
    case CV_8UC4:
        {
            QImage img(mat.data, mat.cols, mat.rows, mat.cols * 4, QImage::Format_ARGB32);
            return img;
        }
    default:
        {
            return QImage();
        }
    }
}
