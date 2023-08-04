#include "widget.h"
#include "ui_widget.h"
#include <QTimer>
#include <QString>
#include <QProcess>
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    qDebug()<<"在电脑跑要有语音播报才行，否则闪退，可以播报部分代码注释掉";
    /************语音播放**************/

    voice_fd = open(uart3, O_RDWR | O_NOCTTY);
    if (voice_fd < 0) {
        printf("open %s failed!\n", uart3);
        return;
    }
    printf("open %s successfully!\n", uart3);
    set_opt(voice_fd, 9600, 8, 'N', 1);

    //语音播放测试成功（先转码再播放）
    formatStr = utf8_to_gbk("小垃圾自助购物系统欢迎您");
    // formatStr = utf8_to_gbk("1");
    sprintf((char *)yuyin,"%s",formatStr);
    SYN_FrameInfo(voice_fd, 0x00, yuyin);
    /************语音播放**************/

    /**********************购物相关初始化***********************/
    std::fill(ChoseItem,ChoseItem+9,0);
    ChoseDelete=-1;
    amount=0;
    ChoseItem_cost[0]=2.5;
    ChoseItem_cost[1]=3;
    ChoseItem_cost[2]=2.5;
    ChoseItem_cost[3]=3;
    ChoseItem_cost[4]=2.5;
    ChoseItem_cost[5]=6;
    ChoseItem_cost[6]=5;
    ChoseItem_cost[7]=1;
    ChoseItem_cost[8]=3.5;
    NumberOrPrice=0;    // 界面标志（购物车/商品单价）

    /**********************视频采集start***********************/
    width = 320;
    hight = 240;
    fps = 0;
    pix = new QPixmap(ui->label->size());
    image = new QImage(width, hight, QImage::Format_RGB888);
//    image  = new QImage(cam_rgb_buf,width,hight,QImage::Format_RGB888);
//    image_replay  = new QImage(cam_rgb_buf_replay,width,hight,QImage::Format_RGB888);
    painter = new QPainter(this);
    timer = new QTimer(this);
    timer->setInterval(1000/24);

    cam_raw_buf_len = width * hight * 4;
    cam_raw_rgb_len = width * hight * 4;
    cam_raw_buf = (unsigned char *)malloc(cam_raw_buf_len);
    if(cam_raw_buf == (void *) -1){
        QMessageBox::critical(this, "ERROR", "malloc ERROR!!!");
        this->close();
    }
    cam_rgb_buf = (unsigned char *)malloc(cam_raw_rgb_len);
    if(cam_rgb_buf == (void *)-1){
        QMessageBox::critical(this, "ERROR", "malloc ERROR!!!");
        this->close();
    }
    cam_vd = new VideoDevice("/dev/video0");
    if(-1 == cam_vd->open_device()){
        QMessageBox::critical(this, "ERROR", "Open Devhighice Error!!!");
        this->close();
    }
    cam_vd->init_device();
    cam_vd->start_capturing();
    connect(timer, &QTimer::timeout, this, [=](){
        int ret = 0;
        ret = cam_vd->get_frame((void **)&cam_raw_buf, (size_t *)&cam_raw_buf_len);
        if(ret < 0){
            qDebug()<<"get_frame error";
            return;
        }
        cam_vd->unget_frame();  // 释放相机帧缓冲区
        yuyv422_to_rgb888(cam_raw_buf, cam_rgb_buf, width, hight);
        *image = QImage(cam_rgb_buf, width, hight, QImage::Format_RGB888);
        *pix = QPixmap::fromImage(*image).scaled(ui->label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->label->setPixmap(*pix);
    });

    timer->start();
    /**********************视频采集end***********************/

}

Widget::~Widget()
{
    delete ui;
    
    /************视频采集**************/
    cam_vd->stop_capturing();
    cam_vd->close_device();
    /************视频采集**************/
}

/****************函数封装start*****************/
/************UTF8字符串转GBK字符串**************/
const char* Widget::utf8_to_gbk (const char* utf8Str){
    utf8String = QString::fromUtf8(utf8Str);    // 将UTF-8编码的字符串即const char*类型的变量转换为QString对象
    gbkCodec = QTextCodec::codecForName("GBK"); // 在Qt框架中获取一个能够处理GBK编码文本的QTextCodec对象
    gbkBytes = gbkCodec->fromUnicode(utf8String);   // 将QString转换为字节流（编码）
    const char *gbkStr = gbkBytes.constData();  // 获取 QByteArray 对象数据的常量指针，通过它可以访问 QByteArray 中的数据，但不能用于修改数据
    return gbkStr;
}
/************UTF8字符串转GBK字符串**************/

/************视频采集**************/
void Widget::paintEvent(QPaintEvent *)  // 回调函数
{
    status = 0;
    /********状态圆点********/
    *pix = QPixmap::fromImage(*image,Qt::AutoColor).scaled(ui->label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPainter painter(pix);

    if(status==0) painter.setBrush(QBrush(Qt::green,Qt::SolidPattern));
    if(status==1) painter.setBrush(QBrush(Qt::red,Qt::SolidPattern));
    if(status==2) painter.setBrush(QBrush(Qt::blue,Qt::SolidPattern));

    painter.drawEllipse(4,4,9,9); //视频图像左上角位置和圆形的宽高
//    painter.end();
    if(status !=2 )
    {

    }
    /********状态圆点********/

    /**********字体*********/
    pen.setColor(Qt::red);
    painter.setPen(pen);
    QFont font("Arial", 8); // 创建所需的字体并设置大小为 10
    painter.setFont(font);
    QFontMetrics fontMetrics(painter.font());
    int textHeight = fontMetrics.height();      // 字体高度
    QPointF textPosition(15, textHeight);       // 左上角位置偏移 10 像素
    painter.drawText(textPosition, "第3组");
    //    painter.drawText(0+10, QFontMetrics(painter.font()).height(), "郭林杰");
    /**********字体*********/

    /********绘制生效********/
    painter.end();
    ui->label->setPixmap(*pix);
    /********绘制生效********/
}

void Widget::yuyv422_to_rgb888(unsigned char *yuyvdata, unsigned char
                               *rgbdata, int w, int h) {
    //码流Y0 U0 Y1 V1 Y2 U2 Y3 V3 --》YUYV像素[Y0 U0 V1] [Y1 U0 V1] [Y2 U2 V3] [Y3 U2 V3]--》RGB像素
    int r1, g1, b1;
    int r2, g2, b2;
    int i;
    for(i=0; i<w*h/2; i++) {
        char data[4];
        memcpy(data, yuyvdata+i*4, 4);
        unsigned char Y0=data[0];
        unsigned char U0=data[1];
        unsigned char Y1=data[2];
        unsigned char V1=data[3];
        r1 = Y0+1.4075*(V1-128);
        if(r1>255)  r1=255;
        if(r1<0)    r1=0;
        g1 =Y0- 0.3455 * (U0-128) - 0.7169*(V1-128);
        if(g1>255)  g1=255;
        if(g1<0)    g1=0;
        b1 = Y0 + 1.779 * (U0-128);
        if(b1>255)  b1=255;
        if(b1<0)    b1=0;
        r2 = Y1+1.4075*(V1-128);
        if(r2>255)  r2=255;
        if(r2<0)    r2=0;
        g2 = Y1- 0.3455 * (U0-128) - 0.7169*(V1-128);
        if(g2>255)  g2=255;
        if(g2<0)    g2=0;
        b2 = Y1 + 1.779 * (U0-128);
        if(b2>255)  b2=255;
        if(b2<0)    b2=0;
        rgbdata[i*6+0]=r1;
        rgbdata[i*6+1]=g1;
        rgbdata[i*6+2]=b1;
        rgbdata[i*6+3]=r2;
        rgbdata[i*6+4]=g2;
        rgbdata[i*6+5]=b2;
    }
}
/************视频采集**************/

/************购物界面交互逻辑**************/
void Widget::ShowAmount()   // 显示购买数量
{
    if(NumberOrPrice==1)
    {
        ui->laPrice1->setText("购买:"+QString::number(ChoseItem[0]));
        ui->laPrice2->setText("购买:"+QString::number(ChoseItem[1]));
        ui->laPrice3->setText("购买:"+QString::number(ChoseItem[2]));
        ui->laPrice6->setText("购买:"+QString::number(ChoseItem[3]));
        ui->laPrice5->setText("购买:"+QString::number(ChoseItem[4]));
        ui->laPrice4->setText("购买:"+QString::number(ChoseItem[5]));
        ui->laPrice7->setText("购买:"+QString::number(ChoseItem[6]));
        ui->laPrice8->setText("购买:"+QString::number(ChoseItem[7]));
        ui->laPrice9->setText("购买:"+QString::number(ChoseItem[8]));
    }
}
void Widget::on_btn_Withdrawal_clicked()    // 取消商品/继续购物切换按钮
{
    if(ChoseDelete==-1) // 取消商品选择
    {
        ui->Line_Printf_Purchase->setText("请选择需要取消的商品");
        formatStr = utf8_to_gbk("请选择需要取消的商品");
        sprintf((char *)yuyin,"%s",formatStr);
        SYN_FrameInfo(voice_fd, 0x00, yuyin);
        ChoseDelete=-2; // 下次切换继续购物
        ui->btn_Withdrawal->setText("继续购物");
    }
    else
    {   // 继续购物选择
        ui->Line_Printf_Purchase->setText("已返回购物系统");
        formatStr = utf8_to_gbk("已返回购物系统");
        sprintf((char *)yuyin,"%s",formatStr);
        SYN_FrameInfo(voice_fd, 0x00, yuyin);
        ui->btn_Withdrawal->setText("取消商品");
        ChoseDelete=-1; // 下次切换取消商品
    }
    ShowAmount();

}
void Widget::on_pushButton_clicked()    //一键下单
{
    for(int i=0;i<9;i++)
    {
        amount=amount+ChoseItem[i]*ChoseItem_cost[i];   // 计费
        ChoseItem[i]=0;     // 清空购物车
    }
    Speak="当前花费为"+QString::number(amount)+"元";
    ui->Line_Printf_Purchase->setText(Speak);
    amount=0;

    utf8Str = Speak.toUtf8().constData();
    formatStr = utf8_to_gbk(utf8Str);
    sprintf((char *)yuyin, "%s", formatStr);
    SYN_FrameInfo(voice_fd, 0x00, yuyin);

    ShowAmount();
}

void Widget::on_btn_ShowWhatYouBuy_clicked()    // 切换购物车/商品单价界面
{
    if(NumberOrPrice==0)
    {   // 购物车界面
        ui->laPrice1->setText("购买:"+QString::number(ChoseItem[0]));
        ui->laPrice2->setText("购买:"+QString::number(ChoseItem[1]));
        ui->laPrice3->setText("购买:"+QString::number(ChoseItem[2]));
        ui->laPrice6->setText("购买:"+QString::number(ChoseItem[3]));
        ui->laPrice5->setText("购买:"+QString::number(ChoseItem[4]));
        ui->laPrice4->setText("购买:"+QString::number(ChoseItem[5]));
        ui->laPrice7->setText("购买:"+QString::number(ChoseItem[6]));
        ui->laPrice8->setText("购买:"+QString::number(ChoseItem[7]));
        ui->laPrice9->setText("购买:"+QString::number(ChoseItem[8]));
        NumberOrPrice=1;    // 下次切换商品单价界面
        ui->btn_ShowWhatYouBuy->setText("显示商品价格");  // 按钮名称
    }
    else
    {   // 商品单价界面
        ui->laPrice1->setText("     "+QString::number(ChoseItem_cost[0]));
        ui->laPrice2->setText("     "+QString::number(ChoseItem_cost[1])+".0");
        ui->laPrice3->setText("     "+QString::number(ChoseItem_cost[2]));
        ui->laPrice6->setText("     "+QString::number(ChoseItem_cost[3])+".0");
        ui->laPrice5->setText("     "+QString::number(ChoseItem_cost[4]));
        ui->laPrice4->setText("     "+QString::number(ChoseItem_cost[5])+".0");
        ui->laPrice7->setText("     "+QString::number(ChoseItem_cost[6])+".0");
        ui->laPrice8->setText("     "+QString::number(ChoseItem_cost[7])+".0");
        ui->laPrice9->setText("     "+QString::number(ChoseItem_cost[8]));
        NumberOrPrice=0;    // 下次切换购物车界面
        ui->btn_ShowWhatYouBuy->setText("显示购物车");
    }


}
// 商品选择触发槽函数
void Widget::on_btn_item_1_clicked()
{
    if(ChoseDelete==-2) // 取消商品标志
    {
        if(ChoseItem[0]>0)  // 检测是否能够删除
        {
            ChoseItem[0]=ChoseItem[0]-1;
            ui->Line_Printf_Purchase->setText("已在购物车中取消一瓶可乐");
            formatStr = utf8_to_gbk("已在购物车中取消一瓶可乐");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
        else    // 购物车中不存在该商品，则无法删除并语音提示
        {
            ui->Line_Printf_Purchase->setText("您没有将可乐选入购物车中");
            formatStr = utf8_to_gbk("您没有将可乐选入购物车中");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
    }
    else    // 选择将该商品加入购物车
    {
        ChoseItem[0]=ChoseItem[0]+1;
        ui->Line_Printf_Purchase->setText("已添加一瓶可乐进购物车");
        formatStr = utf8_to_gbk("已添加一瓶可乐进购物车");
        sprintf((char *)yuyin,"%s",formatStr);
        SYN_FrameInfo(voice_fd, 0x00, yuyin);
    }
    ShowAmount();
}

void Widget::on_btn_item_2_clicked()
{
    if(ChoseDelete==-2)
    {
        if(ChoseItem[1]>0)
        {
            ChoseItem[1]=ChoseItem[1]-1;
            ui->Line_Printf_Purchase->setText("已在购物车中取消一瓶天地一号");
            formatStr = utf8_to_gbk("已在购物车中取消一瓶天地一号");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
        else
        {
            ui->Line_Printf_Purchase->setText("您没有将天地一号选入购物车中");
            formatStr = utf8_to_gbk("您没有将天地一号选入购物车中");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
    }
    else
    {
        ChoseItem[1]=ChoseItem[1]+1;
        ui->Line_Printf_Purchase->setText("已添加一瓶天地一号进购物车");
        formatStr = utf8_to_gbk("已添加一瓶天地一号进购物车");
        sprintf((char *)yuyin,"%s",formatStr);
        SYN_FrameInfo(voice_fd, 0x00, yuyin);
    }
    ShowAmount();
}

void Widget::on_btn_item_3_clicked()
{
    if(ChoseDelete==-2)
    {
        if(ChoseItem[2]>0)
        {
            ChoseItem[2]=ChoseItem[2]-1;
            ui->Line_Printf_Purchase->setText("已在购物车中取消一瓶美年达");
            formatStr = utf8_to_gbk("已在购物车中取消一瓶美年达");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
        else
        {
            ui->Line_Printf_Purchase->setText("您没有将美年达选入购物车中");
            formatStr = utf8_to_gbk("您没有将美年达选入购物车中");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
    }
    else
    {
        ChoseItem[2]=ChoseItem[2]+1;
        ui->Line_Printf_Purchase->setText("已添加一瓶美年达进购物车");
        formatStr = utf8_to_gbk("已添加一瓶美年达进购物车");
        sprintf((char *)yuyin,"%s",formatStr);
        SYN_FrameInfo(voice_fd, 0x00, yuyin);
    }
    ShowAmount();
}

void Widget::on_btn_item_4_clicked()
{
    if(ChoseDelete==-2)
    {
        if(ChoseItem[3]>0)
        {
            ChoseItem[3]=ChoseItem[3]-1;
            ui->Line_Printf_Purchase->setText("已在购物车中取消一瓶王老吉");
            formatStr = utf8_to_gbk("已在购物车中取消一瓶王老吉");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
        else
        {
            ui->Line_Printf_Purchase->setText("您没有将王老吉选入购物车中");
            formatStr = utf8_to_gbk("您没有将王老吉选入购物车中");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
    }
    else
    {
        ChoseItem[3]=ChoseItem[3]+1;
        ui->Line_Printf_Purchase->setText("已添加一瓶王老吉进购物车");
        formatStr = utf8_to_gbk("已添加一瓶王老吉进购物车");
        sprintf((char *)yuyin,"%s",formatStr);
        SYN_FrameInfo(voice_fd, 0x00, yuyin);
    }
    ShowAmount();
}

void Widget::on_btn_item_5_clicked()
{
    if(ChoseDelete==-2)
    {
        if(ChoseItem[4]>0)
        {
            ChoseItem[4]=ChoseItem[4]-1;
            ui->Line_Printf_Purchase->setText("已在购物车中取消一瓶冰红茶");
            formatStr = utf8_to_gbk("已在购物车中取消一瓶冰红茶");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
        else
        {
            ui->Line_Printf_Purchase->setText("您没有将冰红茶选入购物车中");
            formatStr = utf8_to_gbk("您没有将冰红茶选入购物车中");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
    }
    else
    {
        ChoseItem[4]=ChoseItem[4]+1;
        ui->Line_Printf_Purchase->setText("已添加一瓶冰红茶进购物车");
        formatStr = utf8_to_gbk("已添加一瓶冰红茶进购物车");
        sprintf((char *)yuyin,"%s",formatStr);
        SYN_FrameInfo(voice_fd, 0x00, yuyin);
    }
    ShowAmount();
}

void Widget::on_btn_item_6_clicked()
{
    if(ChoseDelete==-2)
    {
        if(ChoseItem[5]>0)
        {
            ChoseItem[5]=ChoseItem[5]-1;
            ui->Line_Printf_Purchase->setText("已在购物车中取消一包薯片");
            formatStr = utf8_to_gbk("已在购物车中取消一包薯片");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
        else
        {
            ui->Line_Printf_Purchase->setText("您没有将薯片选入购物车中");
            formatStr = utf8_to_gbk("您没有将薯片选入购物车中");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
    }
    else
    {
        ChoseItem[5]=ChoseItem[5]+1;
        ui->Line_Printf_Purchase->setText("已添加一包薯片进购物车");
        formatStr = utf8_to_gbk("已添加一包薯片进购物车");
        sprintf((char *)yuyin,"%s",formatStr);
        SYN_FrameInfo(voice_fd, 0x00, yuyin);
    }
    ShowAmount();
}

void Widget::on_btn_item_7_clicked()
{
    if(ChoseDelete==-2)
    {
        if(ChoseItem[6]>0)
        {
            ChoseItem[6]=ChoseItem[6]-1;
            ui->Line_Printf_Purchase->setText("已在购物车中取消一条德芙");
            formatStr = utf8_to_gbk("已在购物车中取消一条德芙");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
        else
        {
            ui->Line_Printf_Purchase->setText("您没有将德芙选入购物车中");
            formatStr = utf8_to_gbk("您没有将德芙选入购物车中");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
    }
    else
    {
        ChoseItem[6]=ChoseItem[6]+1;
        ui->Line_Printf_Purchase->setText("已添加一条德芙进购物车");
        formatStr = utf8_to_gbk("已添加一条德芙进购物车");
        sprintf((char *)yuyin,"%s",formatStr);
        SYN_FrameInfo(voice_fd, 0x00, yuyin);
    }
    ShowAmount();
}

void Widget::on_btn_item_8_clicked()
{
    if(ChoseDelete==-2)
    {
        if(ChoseItem[7]>0)
        {
            ChoseItem[7]=ChoseItem[7]-1;
            ui->Line_Printf_Purchase->setText("已在购物车中取消一袋星球杯");
            formatStr = utf8_to_gbk("已在购物车中取消一袋星球杯");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
        else
        {
            ui->Line_Printf_Purchase->setText("您没有将一袋星球杯选入购物车中");
            formatStr = utf8_to_gbk("您没有将一袋星球杯选入购物车中");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
    }
    else
    {
        ChoseItem[7]=ChoseItem[7]+1;
        ui->Line_Printf_Purchase->setText("已添加一袋星球杯进购物车");
        formatStr = utf8_to_gbk("已添加一袋星球杯进购物车");
        sprintf((char *)yuyin,"%s",formatStr);
        SYN_FrameInfo(voice_fd, 0x00, yuyin);
    }
    ShowAmount();
}

void Widget::on_btn_item_9_clicked()
{
    if(ChoseDelete==-2)
    {
        if(ChoseItem[8]>0)
        {
            ChoseItem[8]=ChoseItem[8]-1;
            ui->Line_Printf_Purchase->setText("已在购物车中取消一包华夫饼");
            formatStr = utf8_to_gbk("已在购物车中取消一包华夫饼");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
        else
        {
            ui->Line_Printf_Purchase->setText("您没有将华夫饼选入购物车中");
            formatStr = utf8_to_gbk("您没有将华夫饼选入购物车中");
            sprintf((char *)yuyin,"%s",formatStr);
            SYN_FrameInfo(voice_fd, 0x00, yuyin);
        }
    }
    else
    {
        ChoseItem[8]=ChoseItem[8]+1;
        ui->Line_Printf_Purchase->setText("已添加一包华夫饼进购物车");
        formatStr = utf8_to_gbk("已添加一包华夫饼进购物车");
        sprintf((char *)yuyin,"%s",formatStr);
        SYN_FrameInfo(voice_fd, 0x00, yuyin);
    }
    ShowAmount();
}
/************购物界面交互逻辑**************/

/************语音播放**************/
int Widget::set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop) {
    struct termios newtio, oldtio;
    if (tcgetattr(fd, &oldtio) != 0) {
        perror("SetupSerial 1");
        return -1;
    }
    bzero(&newtio, sizeof(newtio));

    newtio.c_cflag |= CLOCAL | CREAD;

    newtio.c_cflag &= ~CSIZE;

    switch (nBits) {
    case 5:
        newtio.c_cflag |= CS5;
        break;
    case 6:
        newtio.c_cflag |= CS6;
        break;
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch (nEvent) {
    case 'O':
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':
        newtio.c_cflag &= ~PARENB;
        newtio.c_iflag &= ~INPCK;
        newtio.c_iflag &= ~ISTRIP;
        break;
    }

    switch (nSpeed) {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    case 460800:
        cfsetispeed(&newtio, B460800);
        cfsetospeed(&newtio, B460800);
        break;
    default:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }
    if (nStop == 1)
        newtio.c_cflag &= ~CSTOPB;
    else if (nStop == 2)
        newtio.c_cflag |= CSTOPB;

    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);

    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0) {
        perror("com set error");
        return -1;
    }

    //	printf("set done!\n\r");
    return 0;
}

void Widget::SYN_FrameInfo(int fd, unsigned char Music, unsigned char *HZdata) {
    /****************需要发送的文本**********************************/
    unsigned char Frame_Info[50];
//    unsigned char HZ_Length;
    size_t HZ_Length;
    unsigned char ecc = 0;  // 定义校验字节
    unsigned int i = 0;
    const char* str = reinterpret_cast<const char*>(HZdata);
    HZ_Length = strlen(str);
//    HZ_Length = strlen(HZdata);  // 需要发送文本的长度

    /*****************帧固定配置信息**************************************/
    Frame_Info[0] = 0xFD;               // 构造帧头FD
    Frame_Info[1] = 0x00;               // 构造数据区长度的高字节(数据区长度<=200+3字节)
    Frame_Info[2] = HZ_Length + 3;      // 构造数据区长度的低字节
    Frame_Info[3] = 0x01;               // 构造命令字：合成播放命令
    Frame_Info[4] = 0x01 | Music << 4;  // 构造命令参数：背景音乐设定
    // 0x01为命令参数的字节低3位0、1、2，用于设置文本编码：1：GBK；3：UNICODE编码
    // Music<<4为字节高5位3、4、5、6、7（从第4位开始），0表示不加背景音乐，其他为不同的背景音乐

    /*******************校验码计算***************************************/
    for (i = 0; i < 5; i++) {         // 依次发送构造好的5个帧头字节
        ecc = ecc ^ (Frame_Info[i]);  // 对发送的字节进行异或校验
    }

    for (i = 0; i < HZ_Length; i++) {  // 依次发送待合成的文本数据(<=200字节)
        ecc = ecc ^ (HZdata[i]);       // 对发送的字节进行异或校验
    }
    /*******************发送帧信息***************************************/
    memcpy(&Frame_Info[5], HZdata, HZ_Length);  // 从第6个字节开始
    Frame_Info[5 + HZ_Length] = ecc;            // 最后一个字节

    write(fd, Frame_Info, 5 + HZ_Length + 1);
}
/************语音播放**************/









/****************函数封装end*****************/
