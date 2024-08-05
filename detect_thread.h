#ifndef DETECT_THREAD_H
#define DETECT_THREAD_H

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QVector>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

struct Box {
    cv::Scalar color;
    std::string label;
    cv::Rect rect;
};

class VideoWidget;
class DetectThread: public QThread
{
    Q_OBJECT

public:
    DetectThread(VideoWidget *mainWidget);

protected:
    void run() override;
    void imageDetect(cv::Mat image);

private:
    VideoWidget *mainWidget;
    cv::dnn::Net yoloNet;
    cv::dnn::Net lpNet;
};

#endif // DETECT_THREAD_H
