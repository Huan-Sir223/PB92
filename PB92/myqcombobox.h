#ifndef MYQCOMBOBOX_H
#define MYQCOMBOBOX_H

#include <QObject>
#include <QComboBox>
#include <QWheelEvent>
#include <qabstractitemview.h>
class MyQComboBox:public QComboBox
{
public:
    MyQComboBox(QWidget *parent = nullptr);
protected:
    void wheelEvent(QWheelEvent *event) override {
        if (!view()->isVisible()) { // 如果下拉列表不可见，则忽略滚轮事件
            event->ignore();
        } else {
            QComboBox::wheelEvent(event);
        }
    }
};

#endif // MYQCOMBOBOX_H
