#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    client = new QMqttClient;
    pic_flag = 0; //初始化标志位
    pic_len = 0; //初始化图片大小
    setWindowTitle(tr("Smart home control system"));
    setWindowIcon(QIcon(":/Setting.ico"));
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_pushButton_3_clicked()
{
    client->setHostname("mqtt.yyzlab.com.cn");
    client->setPort(1883);
    client->connectToHost();
    connect(client,SIGNAL(connected()),this,SLOT(link()));
}

void Widget::link()
{
    ui->lineEdit_4->setText("link successful");
}

void Widget::on_pushButton_clicked()
{
    client->publish(ui->lineEdit->text(),ui->lineEdit_2->text().toUtf8());
}

void Widget::on_pushButton_2_clicked()
{
    client->subscribe(ui->lineEdit_3->text());
    connect(client,SIGNAL(messageReceived(QByteArray,QMqttTopicName)),this,SLOT(myReadMessage(QByteArray,QMqttTopicName)));
}

void Widget::myReadMessage(QByteArray buf,QMqttTopicName)
{
    ui->textEdit->append((QString)buf);
    QString str = (QString)buf.mid(2,3);
    if(str == "tem")
    {
        QJsonDocument jDoc = QJsonDocument::fromJson(buf);
        QJsonObject jobj = jDoc.object();
        QJsonValue jval = jobj.value("tem");
        QString data = QString("%1").arg(jval.toDouble());
        ui->lineEdit_5->setText(data);
        QJsonValue jval1 = jobj.value("hum");
        QString data1 = QString("%1").arg(jval1.toDouble());
        ui->lineEdit_6->setText(data1);
    }
}

void Widget::on_pushButton_4_clicked()
{
    QString ip = ui->lineEdit_7->text();
    socket.connectToHost(ip, 8888); //连接主机
    connect(&socket, SIGNAL(readyRead()), this, SLOT(get_pic())); //准备读取数据，如果接收到数据，就转换到槽函数
}
char pic_buf[640*480*3] = {0};
void Widget::get_pic(void)
{
    if(0 == pic_flag)
    {
        if(socket.bytesAvailable() < 10)
            return ; //如果图片长度还没有传送完，则返回

        char pic_size[10] = {0}; //读取服务器传过来的图片长度
        socket.read(pic_size, sizeof(pic_size));
        sscanf(pic_size, "%d", &pic_len); //将字符类型的图片大小转换为无符号整形数大小，并复制给pic_len
        qDebug() << "pic_len" << pic_len << endl;
        pic_flag = 1; //接收完图片长度，重新标志位1，表示下次要接收的数据是图片内容
    }
    else
    {
        if(socket.bytesAvailable() < pic_len)
        {
            return ; //如果图片内容没发完，则返回
        }
        memset(pic_buf, 0, sizeof(pic_buf));
        socket.read(pic_buf, pic_len);

        /*显示图片*/
        QPixmap pix; //QPixmap类用于绘画设备图像显示,支持jpeg格式的图片
        ui->label_7->setScaledContents(true); //设置图片适应窗口大小
        pix.loadFromData((const uchar*)pic_buf, pic_len);
        ui->label_7->setPixmap(pix);
        pic_flag = 0; //标志重置为0，表示下次接收图片的长度
    }
}

void Widget::on_pushButton_5_clicked()
{
    ui->lineEdit_2->setText("{\"lamp\":true,\"id\":0}");
}

void Widget::on_pushButton_6_clicked()
{
    ui->lineEdit_2->setText("{\"fan\":true,\"id\":0}");
}
