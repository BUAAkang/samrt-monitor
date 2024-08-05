#include "widget.h"
#include "ui_widget.h"
#include "video_widget.h"
#include <QVBoxLayout>
#include <QPushButton>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    VideoWidget *videoWidget = new VideoWidget(this);


    QPushButton *pauseButton = new QPushButton("暂停", this);
    connect(pauseButton, &QPushButton::clicked, videoWidget, &VideoWidget::pauseVideo);
    pauseButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;" // 按钮背景色
        "    color: white;" // 按钮文本颜色
        "    border: none;" // 无边框
        "    border-radius: 10px;" // 圆角半径
        "    padding: 10px 20px;" // 内边距
        "    font-size: 16px;" // 字体大小
        "border-style:outset;"
        "}"
        "QPushButton:hover {"
        "    background-color:rgb(50, 155, 50);" // 鼠标悬停时的背景色
        "}"
        "QPushButton:pressed {"
        "    background-color:rgb(255, 128, 64);" // 按下时的背景色
        "}"
        );


    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(videoWidget);
    layout->addWidget(pauseButton);

    // 设置布局到当前窗口
    setLayout(layout);
}

Widget::~Widget()
{
    delete ui;
}
