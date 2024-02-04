#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <SwitchControl.h>
#include <qmessagebox.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_action_triggered()
{
    //tr方便跨平台，兼容性
    QString imgFilePath = QFileDialog::getOpenFileName(this,tr("打开图片"),"C:/","(打开图片(*.jpg *.png *.bmp))");
    if(imgFilePath.isEmpty())
    {
        return;
    }

    //Mat类型，可以看作数组，元组
    srcImage = imread(imgFilePath.toStdString());//转化标准字符串
    //把cv图像对象转换为qimage对象
    cvtColor(srcImage,srcImage,CV_BGR2RGB);//输入对象，输出对象（同名表示覆盖原来的），转换的类型
    //Mat转换为Qimage对象
    QImage displayImg1 = QImage(srcImage.data,srcImage.cols,srcImage.rows,srcImage.cols * srcImage.channels(),QImage::Format_RGB888);//格式化一个8位的
    QImage disimage = imageCenter(displayImg1,ui->before_pic_lbl);
    //显示图片到页面
    ui->before_pic_lbl->setPixmap(QPixmap::fromImage(disimage));
}
QImage MainWindow::imageCenter(QImage qimage,QLabel *qLabel)
{
    QImage image;
    QSize imageSize = qimage.size();
    QSize labelSize = qLabel->size();
    qDebug()<<labelSize.height()<<labelSize.width();
    double dWidthRatio = 1.0 * imageSize.width() / labelSize.width();
    double dHeightRatio = 1.0 * imageSize.height() / labelSize.height();
    if (dWidthRatio>dHeightRatio)
    {
        image = qimage.scaledToWidth(labelSize.width());
    }
    else
    {
        image = qimage.scaledToHeight(labelSize.height());
    }
    return image;
}

//图片 预处理
Mat MainWindow::mat_to_samples(Mat &image)
{
    int w = image.cols;
    int h = image.rows;
    int samplecount = w*h;
    int dims = image.channels();
    Mat points(samplecount, dims, CV_32F, Scalar(10));

    int index = 0;
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            index = row*w + col;
            Vec3b bgr = image.at<Vec3b>(row, col);
            points.at<float>(index, 0) = static_cast<int>(bgr[0]);
            points.at<float>(index, 1) = static_cast<int>(bgr[1]);
            points.at<float>(index, 2) = static_cast<int>(bgr[2]);
        }
    }
    return points;
}

//背景替换函数实现
QImage MainWindow::BackroundReplace(Mat &img,int R,int G,int B)
{
    // 组装数据
    Mat points = mat_to_samples(img);

    // 运行KMeans
    int numCluster = 4;
    Mat labels;
    Mat centers;
    TermCriteria criteria = TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 10, 0.1);
    kmeans(points, numCluster, labels, criteria, 3, KMEANS_PP_CENTERS, centers);

    // 去背景+遮罩生成
    Mat mask=Mat::zeros(img.size(), CV_8UC1);
    int index = img.rows*2 + 2;
    int cindex = labels.at<int>(index, 0);
    int height = img.rows;
    int width = img.cols;
    //Mat dst;
    //src.copyTo(dst);
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            index = row*width + col;
            int label = labels.at<int>(index, 0);
            if (label == cindex) { // 背景
                //dst.at<Vec3b>(row, col)[0] = 0;
                //dst.at<Vec3b>(row, col)[1] = 0;
                //dst.at<Vec3b>(row, col)[2] = 0;
                mask.at<uchar>(row, col) = 0;
            } else {
                mask.at<uchar>(row, col) = 255;
            }
        }
    }
    //imshow("mask", mask);

    // 腐蚀 + 高斯模糊
    Mat k = getStructuringElement(MORPH_RECT, Size(3, 3), Point(-1, -1));
    erode(mask, mask, k);
    //imshow("erode-mask", mask);
    GaussianBlur(mask, mask, Size(3, 3), 0, 0);
    //imshow("Blur Mask", mask);

    // 通道混合
    RNG rng(12345);
    Vec3b color;
    //引入形参B G R
    color[0] = R;//rng.uniform(0, 255);
    color[1] = G;// rng.uniform(0, 255);
    color[2] = B;// rng.uniform(0, 255);
    Mat result(img.size(), img.type());

    double w = 0.0;
    int b = 0, g = 0, r = 0;
    int b1 = 0, g1 = 0, r1 = 0;
    int b2 = 0, g2 = 0, r2 = 0;

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int m = mask.at<uchar>(row, col);
            if (m == 255) {
                result.at<Vec3b>(row, col) = img.at<Vec3b>(row, col); // 前景
            }
            else if (m == 0) {
                result.at<Vec3b>(row, col) = color; // 背景
            }
            else {
                w = m / 255.0;
                b1 = img.at<Vec3b>(row, col)[0];
                g1 = img.at<Vec3b>(row, col)[1];
                r1 = img.at<Vec3b>(row, col)[2];

                b2 = color[0];
                g2 = color[1];
                r2 = color[2];

                b = b1*w + b2*(1.0 - w);
                g = g1*w + g2*(1.0 - w);
                r = r1*w + r2*(1.0 - w);

                result.at<Vec3b>(row, col)[0] = b;
                result.at<Vec3b>(row, col)[1] = g;
                result.at<Vec3b>(row, col)[2] = r;
            }
        }
    }
    //cvtColor(result,result,CV_BGR2RGB);//输入对象，输出对象（同名表示覆盖原来的），转换的类型
    //复制一份result,用于后续叠加处理。
    result.copyTo(srcImage);
    QImage displayImg1 =  QImage(result.data,result.cols,result.rows,result.cols * result.channels(),QImage::Format_RGB888);//格式化一个8位的
    //返回处理好的QImage,返回copy否则为黑色
    return displayImg1.copy();

}

//判断字符串是否是数字
bool MainWindow::isNum(const QString &str)
{
    for(int i=0;i<str.length();i++)
    {
        if(isdigit(str.toStdString()[i])==false)
        {
            return false;
        }
    }
    return true;
}




void MainWindow::on_radioButton_toggled(bool checked)
{
    if(checked==true)
    {
        back_color = "blue";
    }
}


void MainWindow::on_radioButton_2_toggled(bool checked)
{
    if(checked==true)
    {
        back_color = "red";
        qDebug()<<back_color;
    }
}


void MainWindow::on_radioButton_3_toggled(bool checked)
{
    if(checked==true)
    {
        back_color = "white";
    }
}




void MainWindow::on_pushButton_clicked()
{
    qDebug()<<back_color;

    if(back_color=="white")
    {
        if (srcImage.empty()) {
            printf("could not load image...\n");
            return;
        }

        //调用背景替换函数 白色BGR对应 255 255 255
        QImage displayImg1 = BackroundReplace(srcImage,255,255,255);
        //displayImg用于保存使用
        displayImg = displayImg1.copy();//此处不要使用浅拷贝，否则会得到全黑图像。
        //适应窗口
        QImage disimage = imageCenter(displayImg1,ui->after_pic_lbl);
        //输出到QLabel
        ui->after_pic_lbl->setPixmap(QPixmap::fromImage(disimage));
        //完成提示
       // QMessageBox:: StandardButton msg=  QMessageBox::information(NULL, "完成","无问题请及时保存",QMessageBox::Yes);



    }
    if(back_color=="blue")
    {

            if (srcImage.empty()) {
                printf("could not load image...\n");
                return;
            }

            //调用背景替换函数蓝色BGR对应255，0，0
            QImage displayImg1 = BackroundReplace(srcImage,0,0,255);
            //displayImg用于保存使用
            displayImg = displayImg1.copy();//此处不要使用浅拷贝，否则会得到全黑图像。
            //适应窗口
            QImage disimage = imageCenter(displayImg1,ui->after_pic_lbl);
            //输出到QLabel
            ui->after_pic_lbl->setPixmap(QPixmap::fromImage(disimage));
            //完成提示
           // QMessageBox:: StandardButton msg=  QMessageBox::information(NULL, "完成","无问题请及时保存",QMessageBox::Yes);


    }
    if(back_color=="red")
    {
        if (srcImage.empty()) {
            printf("could not load image...\n");
            return;
        }

        //调用背景替换函数 红色BGR对于 0 0 255
        QImage displayImg1 = BackroundReplace(srcImage,255,0,0);
        //displayImg用于保存使用
        displayImg = displayImg1.copy();//此处不要使用浅拷贝，否则会得到全黑图像。
        //适应窗口
        QImage disimage = imageCenter(displayImg1,ui->after_pic_lbl);
        //输出到QLabel
        ui->after_pic_lbl->setPixmap(QPixmap::fromImage(disimage));
        //完成提示
        //QMessageBox:: StandardButton msg=  QMessageBox::information(NULL, "完成","无问题请及时保存",QMessageBox::Yes);

    }
    if(back_color=="custom")
    {
        if (srcImage.empty()) {
            printf("could not load image...\n");
            return;
        }

        //调用背景替换函数 自定义BGR对于 B G R
        QImage displayImg1 = BackroundReplace(srcImage,R,G,B);
        //displayImg用于保存使用
        displayImg = displayImg1.copy();//此处不要使用浅拷贝，否则会得到全黑图像。
        //适应窗口
        QImage disimage = imageCenter(displayImg1,ui->after_pic_lbl);
        //输出到QLabel
        ui->after_pic_lbl->setPixmap(QPixmap::fromImage(disimage));
        //完成提示
        //QMessageBox:: StandardButton msg=  QMessageBox::information(NULL, "完成","无问题请及时保存",QMessageBox::Yes);

    }

    //处理美化
    if(isNoisy)
    {

        if (srcImage.empty()) {
            printf("could not load image...\n");
            return;
        }
        //使用中值滤波去噪
        medianBlur(srcImage, srcImage, 3);
        QImage displayImg1 =  QImage(srcImage.data,srcImage.cols,srcImage.rows,srcImage.cols * srcImage.channels(),QImage::Format_RGB888);
        displayImg = displayImg1.copy();//此处不要使用浅拷贝，否则会得到全黑图像。
        //适应窗口
        QImage disimage = imageCenter(displayImg1,ui->after_pic_lbl);
        //输出到QLabel
        ui->after_pic_lbl->setPixmap(QPixmap::fromImage(disimage));

    }
    if(isBeauty)
    {
        if (srcImage.empty()) {
            printf("could not load image...\n");
            return;
        }
        //转为c8u3
        //srcImage.convertTo(srcImage, CV_8UC3);
        //使用双边滤波去噪
        //Mat scrImage111;
        bilateralFilter(srcImage, srcImage1, 15, 100, 2);
        //增加对比度
        Mat kernel = (Mat_<int>(3, 3) << 0, -1, 0, -1, 5, -1, 0, -1, 0);
        filter2D(srcImage1, srcImage1, -1, kernel, Point(-1, -1), 0);
        QImage displayImg1 =  QImage(srcImage1.data,srcImage1.cols,srcImage1.rows,srcImage1.cols * srcImage1.channels(),QImage::Format_RGB888);
        displayImg = displayImg1.copy();//此处不要使用浅拷贝，否则会得到全黑图像
        //适应窗口
        QImage disimage = imageCenter(displayImg1,ui->after_pic_lbl);
        //输出到QLabel
        ui->after_pic_lbl->setPixmap(QPixmap::fromImage(disimage));

    }
    //完成提示
    QMessageBox:: StandardButton msg=  QMessageBox::information(NULL, "完成","如无问题请及时保存",QMessageBox::Yes);

}





void MainWindow::on_pushButton_2_clicked()
{
    QImage disimage = imageCenter(displayImg,ui->after_pic_lbl);
    ui->after_pic_lbl->setPixmap(QPixmap::fromImage(disimage));
    if(displayImg.isNull())
    {
        QMessageBox::information(NULL, "错误","未处理无法保存",QMessageBox::Yes);
        return;
    }
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "保存图片",
                                                    "./",
                                                    "图片(*.jpg");


    qDebug()<<filename<<displayImg.isNull();
    bool saved = QPixmap::fromImage(displayImg).save(filename,"jpg",100);

    if(saved)
    {
        QMessageBox::information(NULL, "完成","保存成功",QMessageBox::Yes);
    }
    else
    {
        QMessageBox::information(NULL, "错误","保存失败",QMessageBox::Yes);
    }
    qDebug()<<filename;

    //qDebug()<<"fffff";
}


void MainWindow::on_radioButton_4_toggled(bool checked)
{
    if(checked)
    {
        //自定义模式
        back_color="custom";
    }
}


//R
void MainWindow::on_lineEdit_textEdited(const QString &arg1)
{
    //是否是数字

    bool isnum = isNum(arg1);
    qDebug()<<isnum;
    if(isnum)
    {
        //是否是符合规范的数字
        if(arg1.length()>3)
        {
            QMessageBox::information(NULL, "错误","输入不规范",QMessageBox::Yes);
            this->ui->lineEdit->setText("0");
            return;
        }
    }
    else
    {
        QMessageBox::information(NULL, "错误","输入不规范",QMessageBox::Yes);
        this->ui->lineEdit->setText("0");
        return;
    }



    R = arg1.toInt();

}

//G
void MainWindow::on_lineEdit_2_textEdited(const QString &arg1)
{
    //是否是数字

    bool isnum = isNum(arg1);
    //qDebug()<<isnum;
    if(isnum)
    {
        //是否是符合规范的数字
        if(arg1.length()>3)
        {
            QMessageBox::information(NULL, "错误","输入不规范",QMessageBox::Yes);
            this->ui->lineEdit_2->setText("0");
            return;
        }
    }
    else
    {
        QMessageBox::information(NULL, "错误","输入的不是数字",QMessageBox::Yes);
        this->ui->lineEdit_2->setText("0");
        return;
    }
    G = arg1.toInt();
}

//B
void MainWindow::on_lineEdit_3_textEdited(const QString &arg1)
{
    //是否是数字
    bool isnum = isNum(arg1);

    if(isnum)
    {
        //是否是符合规范的数字
        if(arg1.length()>3)
        {
            QMessageBox::information(NULL, "错误","输入不规范",QMessageBox::Yes);
            this->ui->lineEdit_3->setText( "0");
            return;
        }
    }
    else
    {
        QMessageBox::information(NULL, "错误","输入不规范",QMessageBox::Yes);
        this->ui->lineEdit_3->setText( "0");
        return;
    }
    B = arg1.toInt();
}


void MainWindow::on_checkBox_toggled(bool checked)
{
    if(checked)
    {
        isNoisy=true;
    }
}


void MainWindow::on_checkBox_2_toggled(bool checked)
{
    if(checked)
    {
        isBeauty=true;
    }
}

