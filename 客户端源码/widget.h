#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtMqtt/qmqttclient.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTcpSocket>
namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_pushButton_3_clicked();
    void link();
    void get_pic();
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();
    void myReadMessage(QByteArray buf,QMqttTopicName);

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_6_clicked();

private:
    Ui::Widget *ui;
    QMqttClient *client;
    QTcpSocket socket; //网络套接字
    int pic_flag; //标记接收的是图片的长度0，还是照片信息1
    int pic_len; //记录图片大小
};

#endif // WIDGET_H
