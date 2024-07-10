#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QMutex>
#include <opencv2/opencv.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}

class VideoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoWidget(QWidget *parent = nullptr);
    ~VideoWidget();

public slots:
    void pauseVideo();
protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onFrameDecoded();

private:
    void decodeVideo();
    QImage matToQImage(const cv::Mat &mat);

    QImage currentImage;
    QTimer *timer;
    QMutex mutex;

    AVFormatContext *formatContext = nullptr;
    AVCodecContext *codecContext = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *packet = nullptr;
    SwsContext *swsContext = nullptr;
    int videoStreamIndex = -1;
    bool isPaused;
};

#endif // VIDEOWIDGET_H
