#include "widget.h"
#include "ui_widget.h"
#include "VideoWidget.h"
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

    // 创建一个垂直布局并添加 VideoWidget 和暂停按钮
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
