#ifndef VIDEO_WIDGET_H
#define VIDEO_WIDGET_H

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QMutex>
#include "detect_thread.h"

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

    cv::Mat curMat;
    std::vector<Box> curBoxes;

public slots:
    void pauseVideo();
protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onFrameDecoded();

private:
    void processDetection (cv::Mat &mat, const cv::Mat &yoloDetections, const cv::Mat &lpDetections);
    void decodeVideo();
    QImage matToQImage(const cv::Mat &mat);

    QImage currentImage;
    QTimer *timer;

    AVFormatContext *formatContext = nullptr;
    AVCodecContext *codecContext = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *packet = nullptr;
    SwsContext *swsContext = nullptr;
    int videoStreamIndex = -1;
    bool isPaused;

    DetectThread *detectThread;
};

#endif // VIDEO_WIDGET_H
