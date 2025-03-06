#ifndef SCAMERA_H
#define SCAMERA_H

#include <QWidget>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <QCameraInfo>
#include <QFileDialog>
#include <QtConcurrent>

#include <qdebug.h>
using namespace cv;
namespace Ui {
class scamera;
}

class scamera : public QWidget
{
    Q_OBJECT
signals:
    void updateImage(QImage img);
    void sent_delete();
public:
    explicit scamera(QWidget *parent = 0);
    ~scamera();
public slots:
    void rev_slot(int str);
    void readImage();
    void quitThread();

    QImage MatToQImage(const Mat& mat);
private:
    VideoCapture* m_cap = nullptr;
    Mat mat;
 bool m_play = false;
    Ui::scamera *ui;
};

#endif // SCAMERA_H
