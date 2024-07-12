#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QImage>
#include <QTimer>
#include <QMutex>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
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
    void processDetection (cv::Mat &mat, const cv::Mat &detections);
    void decodeVideo();
    void generateDummyDetections(cv::Mat &frame, cv::Mat &detections);
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


    cv::dnn::Net yoloNet;
    cv::dnn::Net faceNet;
    cv::dnn::Net vehicleNet;
    cv::dnn::Net licensePlateNet;

};

#endif // VIDEOWIDGET_H
