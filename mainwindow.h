#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QLabel>
#include <QMainWindow>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <QToolButton>
#include <QApplication>
#include <QSpinBox>
#include <QTextEdit>
#include <QFileDialog>
#include <opencv2/core/core.hpp>
#include <QDebug>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include "opencv2/imgproc/imgproc_c.h"
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
//usleep
#include <unistd.h>
#include <iostream>
#include <QMainWindow>
using namespace cv;
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QImage imageCenter(QImage qimage,QLabel *qLabel);
    Mat mat_to_samples(Mat &image);

private slots:
    void on_action_triggered();

    void on_radioButton_4_toggled(bool checked);

    void on_radioButton_toggled(bool checked);

    void on_radioButton_2_toggled(bool checked);

    void on_radioButton_3_toggled(bool checked);

    void on_lineEdit_textEdited(const QString &arg1);

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;
    Mat srcImage;//记录原图的变量，cv命名空间下的Mat类型
    //auto result;
    //qint16 pic_cun;//图片尺寸
    QString back_color;//背景颜色
    //qint32 pic_size;//图片大小
    //qint32 pic_size_after;//压缩图片至
    QImage displayImg;
    //QPixmap disimg;

};
#endif // MAINWINDOW_H
