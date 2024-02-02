#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    if(back_color=="white")
    {
        if (srcImage.empty()) {
            printf("could not load image...\n");
            return;
        }

        // 组装数据
        Mat points = mat_to_samples(srcImage);

        // 运行KMeans
        int numCluster = 4;
        Mat labels;
        Mat centers;
        TermCriteria criteria = TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 10, 0.1);
        kmeans(points, numCluster, labels, criteria, 3, KMEANS_PP_CENTERS, centers);

        // 去背景+遮罩生成
        Mat mask=Mat::zeros(srcImage.size(), CV_8UC1);
        int index = srcImage.rows*2 + 2;
        int cindex = labels.at<int>(index, 0);
        int height = srcImage.rows;
        int width = srcImage.cols;
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
        color[0] = 255;//rng.uniform(0, 255);
        color[1] = 255;// rng.uniform(0, 255);
        color[2] = 255;// rng.uniform(0, 255);
        Mat result(srcImage.size(), srcImage.type());

        double w = 0.0;
        int b = 0, g = 0, r = 0;
        int b1 = 0, g1 = 0, r1 = 0;
        int b2 = 0, g2 = 0, r2 = 0;

        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                int m = mask.at<uchar>(row, col);
                if (m == 255) {
                    result.at<Vec3b>(row, col) = srcImage.at<Vec3b>(row, col); // 前景
                }
                else if (m == 0) {
                    result.at<Vec3b>(row, col) = color; // 背景
                }
                else {
                    w = m / 255.0;
                    b1 = srcImage.at<Vec3b>(row, col)[0];
                    g1 = srcImage.at<Vec3b>(row, col)[1];
                    r1 = srcImage.at<Vec3b>(row, col)[2];

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
        //Mat转换为Qimage对象
        QImage displayImg1 =  QImage(result.data,result.cols,result.rows,result.cols * result.channels(),QImage::Format_RGB888);//格式化一个8位的
        displayImg = displayImg1.copy();//此处不要使用浅拷贝，否则会得到全黑图像。

        QImage disimage = imageCenter(displayImg1,ui->after_pic_lbl);
        ui->after_pic_lbl->setPixmap(QPixmap::fromImage(disimage));
    }
    else if(back_color=="blue")
    {

            if (srcImage.empty()) {
                printf("could not load image...\n");
                return;
            }

            // 组装数据
            Mat points = mat_to_samples(srcImage);

            // 运行KMeans
            int numCluster = 4;
            Mat labels;
            Mat centers;
            TermCriteria criteria = TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 10, 0.1);
            kmeans(points, numCluster, labels, criteria, 3, KMEANS_PP_CENTERS, centers);

            // 去背景+遮罩生成
            Mat mask=Mat::zeros(srcImage.size(), CV_8UC1);
            int index = srcImage.rows*2 + 2;
            int cindex = labels.at<int>(index, 0);
            int height = srcImage.rows;
            int width = srcImage.cols;
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
            color[0] = 0;//rng.uniform(0, 255);
            color[1] = 0;// rng.uniform(0, 255);
            color[2] = 255;// rng.uniform(0, 255);
            Mat result(srcImage.size(), srcImage.type());

            double w = 0.0;
            int b = 0, g = 0, r = 0;
            int b1 = 0, g1 = 0, r1 = 0;
            int b2 = 0, g2 = 0, r2 = 0;

            for (int row = 0; row < height; row++) {
                for (int col = 0; col < width; col++) {
                    int m = mask.at<uchar>(row, col);
                    if (m == 255) {
                        result.at<Vec3b>(row, col) = srcImage.at<Vec3b>(row, col); // 前景
                    }
                    else if (m == 0) {
                        result.at<Vec3b>(row, col) = color; // 背景
                    }
                    else {
                        w = m / 255.0;
                        b1 = srcImage.at<Vec3b>(row, col)[0];
                        g1 = srcImage.at<Vec3b>(row, col)[1];
                        r1 = srcImage.at<Vec3b>(row, col)[2];

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
            //Mat转换为Qimage对象
            QImage displayImg1 =  QImage(result.data,result.cols,result.rows,result.cols * result.channels(),QImage::Format_RGB888);//格式化一个8位的
            displayImg = displayImg1.copy();//此处不要使用浅拷贝，否则会得到全黑图像。

            QImage disimage = imageCenter(displayImg1,ui->after_pic_lbl);
            //显示图片到页面
            ui->after_pic_lbl->setPixmap(QPixmap::fromImage(disimage));

}
    else if(back_color=="red")
    {
            if (srcImage.empty()) {
                printf("could not load image...\n");
                return;
            }

            // 组装数据
            Mat points = mat_to_samples(srcImage);

            // 运行KMeans
            int numCluster = 4;
            Mat labels;
            Mat centers;
            TermCriteria criteria = TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 10, 0.1);
            kmeans(points, numCluster, labels, criteria, 3, KMEANS_PP_CENTERS, centers);

            // 去背景+遮罩生成
            Mat mask=Mat::zeros(srcImage.size(), CV_8UC1);
            int index = srcImage.rows*2 + 2;
            int cindex = labels.at<int>(index, 0);
            int height = srcImage.rows;
            int width = srcImage.cols;
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
            color[0] = 255;//rng.uniform(0, 255);
            color[1] = 0;// rng.uniform(0, 255);
            color[2] = 0;// rng.uniform(0, 255);
            Mat result(srcImage.size(), srcImage.type());


            double w = 0.0;
            int b = 0, g = 0, r = 0;
            int b1 = 0, g1 = 0, r1 = 0;
            int b2 = 0, g2 = 0, r2 = 0;

            for (int row = 0; row < height; row++) {
                for (int col = 0; col < width; col++) {
                    int m = mask.at<uchar>(row, col);
                    if (m == 255) {
                        result.at<Vec3b>(row, col) = srcImage.at<Vec3b>(row, col); // 前景
                    }
                    else if (m == 0) {
                        result.at<Vec3b>(row, col) = color; // 背景
                    }
                    else {
                        w = m / 255.0;
                        b1 = srcImage.at<Vec3b>(row, col)[0];
                        g1 = srcImage.at<Vec3b>(row, col)[1];
                        r1 = srcImage.at<Vec3b>(row, col)[2];

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


            //qDebug()<<typeToString(result)<<"qwe";
            //cvtColor(result,result,CV_BGR2RGB);//输入对象，输出对象（同名表示覆盖原来的），转换的类型
            //Mat转换为Qimage对象
            //resultImg = result.convertTo(Mat);
            QImage displayImg1 =  QImage(result.data,result.cols,result.rows,result.cols * result.channels(),QImage::Format_RGB888);//格式化一个8位的
            displayImg = displayImg1.copy();//此处不要使用浅拷贝，否则会得到全黑图像。

            QImage disimage = imageCenter(displayImg1,ui->after_pic_lbl);
            //显示图片到页面
            //QPixmap::fromImage(displayImg).save("C:/Users/qwe/Desktop/111222.jpg","jpg",100);
            ui->after_pic_lbl->setPixmap(QPixmap::fromImage(disimage));
            //QMessageBox::information(this,"完成","是否保存到当前目录？"+pbtn_name,"是","否",0,1);
            //QMessageBox::information(NULL, "完成","Content",QMessageBox::Yes | QMessageBox::No);
            QMessageBox:: StandardButton msg=  QMessageBox::information(NULL, "完成","请及时保存",QMessageBox::Yes);


}

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

    qDebug()<<"fffff";
}

