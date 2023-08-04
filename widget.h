#ifndef WIDGET_H
#define WIDGET_H

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <QWidget>
#include "videodevice.h"
#include <QImage>
#include <QTimer>
#include <QDateTime>
#include <QPixmap>
#include <QMessageBox>
#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QApplication>
#include <QProcess>
#include <QTextCodec>
#include <QString>
#include <QApplication>
#include <cstdio>
#include <QTimer>


QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    /**********************语音播放***********************/

    int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop);
    void SYN_FrameInfo(int fd,unsigned char Music,unsigned char *HZdata);

    unsigned char yuyin[50];
//    QString yuyin;

    int voice_fd;
    const char *uart3 = "/dev/ttySAC0";
    /**********************语音播放***********************/

    /**********************Purchase***********************/
    int ChoseItem[9];
    float ChoseItem_cost[9];
    float amount;
    QString Speak;
    QProcess text;
    int NumberOrPrice;
    int ChoseDelete;

    const char* formatStr;
    const char *utf8Str, *gbkStr;   // gbk编码字符串类型
    QString utf8String, gbkString;  // gbk编码QString类型
    QByteArray gbkBytes;
    QTextCodec* utf8Codec;
    QTextCodec* gbkCodec;
    QByteArray utf8Bytes;

    const char* utf8_to_gbk (const char* utf8Str);
    /**********************Purchase***********************/


    /**********************视频采集***********************/
    VideoDevice *cam_vd;

    unsigned char *cam_raw_buf;
    unsigned char *cam_rgb_buf;
    unsigned int cam_raw_buf_len;
    unsigned int cam_raw_rgb_len;

    unsigned char *cam_raw_buf_replay;
    unsigned int  cam_raw_buf_replay_len;
    unsigned char *cam_rgb_buf_replay;
    unsigned int  cam_rgb_buf_replay_len;

    unsigned int width;
    unsigned int hight;

    QImage *image;
    QImage *image_replay;
    QTimer *timer;
    QPixmap *pix;
    QPainter *painter;
    QPen pen;
    int fps;
    QDateTime preDateTime,curDateTime;
    char status;//0 green 1 red 2 blue


    void yuyv422_to_rgb888(unsigned char *yuyvdaya, unsigned char *rgbdata, int w, int h);
    void paintEvent(QPaintEvent *);
    /**********************视频采集***********************/

private slots:

    void on_btn_item_1_clicked();

    void on_btn_item_2_clicked();

    void on_btn_item_3_clicked();

    void on_btn_item_4_clicked();

    void on_btn_item_5_clicked();

    void on_btn_item_6_clicked();

    void on_btn_item_7_clicked();

    void on_btn_item_8_clicked();

    void on_btn_item_9_clicked();

    void on_btn_Withdrawal_clicked();

    void on_pushButton_clicked();

    void on_btn_ShowWhatYouBuy_clicked();

    void ShowAmount();


private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
