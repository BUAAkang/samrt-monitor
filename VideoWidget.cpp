#include "VideoWidget.h"
#include <QPainter>
#include <QDebug>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>


VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent),
    timer(new QTimer(this)),
    formatContext(nullptr),
    codecContext(nullptr),
    frame(av_frame_alloc()),
    packet(av_packet_alloc()),
    swsContext(nullptr),
    videoStreamIndex(-1),
    isPaused(false),
    recogCnt(10),
    detectionsCache(cv::Mat())
{
    av_log_set_level(AV_LOG_INFO);
    avformat_network_init();

    // 打开视频流
    const char *videoUrl = "rtsp://192.168.1.26:554/stream0";
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "buffer_size", "90048576", 0); // 设置缓冲区大小为2MB

    if (avformat_open_input(&formatContext, videoUrl, nullptr, &opts) != 0) {
        qDebug() << "Could not open input";
        av_dict_free(&opts); // 确保在失败时释放字典
        return;
    }
    av_dict_free(&opts); // 确保成功时释放字典


    // 查找流信息
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        qDebug() << "Could not find stream information";
        return;
    }

    // 查找视频流索引
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        qDebug() << "Could not find video stream";
        return;
    }

    // 查找解码器
    AVCodecParameters *codecParams = formatContext->streams[videoStreamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);

    if (!codec) {
        qDebug() << "Unsupported codec";
        return;
    }

    // 初始化解码器上下文
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        qDebug() << "Could not allocate video codec context";
        return;
    }

    if (avcodec_parameters_to_context(codecContext, codecParams) < 0) {
        qDebug() << "Could not copy codec parameters to context";
        return;
    }

    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        qDebug() << "Could not open codec";
        return;
    }

    // 初始化视频转换上下文
    swsContext = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt,
                                codecContext->width, codecContext->height, AV_PIX_FMT_BGR24,
                                SWS_BILINEAR, nullptr, nullptr, nullptr);



    yoloNet = cv::dnn::readNetFromONNX("C:/Users/Administrator/PycharmProjects/pythonProject/yolov8n-oiv7.onnx");

    // 检查模型加载是否成功
    if (yoloNet.empty()) {
        qDebug() << "Failed to load  model!";
        return;
    }

    connect(timer, &QTimer::timeout, this, &VideoWidget::onFrameDecoded);
    timer->start(30);

}

VideoWidget::~VideoWidget()
{
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
    av_frame_free(&frame);
    av_packet_free(&packet);
    sws_freeContext(swsContext);
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QMutexLocker locker(&mutex);

    if (!currentImage.isNull()) {
        painter.drawImage(0, 0, currentImage);
    }
}

void VideoWidget::onFrameDecoded()
{
    // Judge if this frame is recog frame
    bool flag = false;
    if(recogCnt == 10){
        flag = true;
        recogCnt = 0;
    }
    recogCnt++;

    if (isPaused) return;
    if (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(codecContext, packet) == 0) {
                if (avcodec_receive_frame(codecContext, frame) == 0) {
                    AVFrame *rgbFrame = av_frame_alloc();
                    rgbFrame->format = AV_PIX_FMT_RGB24;
                    rgbFrame->width = codecContext->width;
                    rgbFrame->height = codecContext->height;

                    if (av_frame_get_buffer(rgbFrame, 32) < 0) {
                        qDebug() << "Could not allocate frame buffer";
                        av_frame_free(&rgbFrame);
                        return;
                    }

                    sws_scale(swsContext, frame->data, frame->linesize, 0, codecContext->height, rgbFrame->data, rgbFrame->linesize);

                    cv::Mat mat(codecContext->height, codecContext->width, CV_8UC3, rgbFrame->data[0], rgbFrame->linesize[0]);

                    // std:: cout << "height: " << codecContext->height << " width: " << codecContext->width << std::endl;

                    if(flag == true){
                        // 创建 blob，并设置为模型的输入
                        cv::Mat blob = cv::dnn::blobFromImage(mat, 1.0 / 255.0, cv::Size(640, 640), cv::Scalar(0, 0, 0), true, false);
                        yoloNet.setInput(blob);
                        // 执行前向传播，获取检测结果
                        cv::Mat detections = yoloNet.forward();
                        detectionsCache = detections.clone();
                        // 打印模型的输出
                        // std::cout << "Model Output: " << detections << std::endl;
                        // 处理检测结果，并绘制检测框
                        processDetection(mat, detections);
                    }
                    else{
                        if(!detectionsCache.empty()){
                            // std::cout << "tag here" << std::endl;
                            processDetection(mat, detectionsCache);
                        }
                    }

                    QImage img = matToQImage(mat);

                    {
                        QMutexLocker locker(&mutex);
                        currentImage = img;
                    }

                    av_frame_free(&rgbFrame);

                    update(); // 触发重新绘制
                }
            }
        }
        av_packet_unref(packet);
    }
}

QImage VideoWidget::matToQImage(const cv::Mat &mat)
{
    if (mat.type() == CV_8UC1) {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
    } else {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888).rgbSwapped();
    }
}
void VideoWidget::pauseVideo()
{
    isPaused = !isPaused; // 切换暂停状态

}
void VideoWidget::processDetection(cv::Mat &image, const cv::Mat &detections)
{
    // Global var
    int index[3] = {94, 326, 572}; // car, human, lincense
    float conf[3] = {0.5, 0.5, 1e-2};

    double image_width = image.cols;
    double image_height = image.rows;
    double scaled_width = 640;
    double scaled_height = 640;

    std::vector<cv::Rect> boxes;
    std::vector<int> classIds;
    std::vector<float> confidences;

    // std:: cout << "height: " << image_height << " width: " << image_width << std::endl;

    for (int i = 0; i < 8400; ++i) {
        // judge target rect
        int mode = -1;

        for(int j = 0; j < 3; j++){
            // std::cout << detections.at<float>(0, index[j], i) << std::endl;
            if(detections.at<float>(0, index[j], i) > conf[j])
                mode = j;
        }
        if (mode == -1)
            continue;

        float centerX = detections.ptr<float>(0, 0)[i]* (image_width / scaled_width);
        float centerY = detections.ptr<float>(0, 1)[i]* (image_height / scaled_height);
        float width = detections.ptr<float>(0, 2)[i]* (image_width / scaled_width);
        float height = detections.ptr<float>(0, 3)[i]* (image_height / scaled_height);

        float left = centerX - width / 2;
        float top = centerY - height / 2;
        cv::Rect rect(left, top, width, height);

        boxes.push_back(rect);
        classIds.push_back(mode);
        confidences.push_back(detections.at<float>(0, index[mode], i));
    }

    std::vector<int> indices;
    float nmsThreshold = 0.4;
    cv::dnn::NMSBoxes(boxes, confidences, 0.0, nmsThreshold, indices);

    for (size_t i = 0; i < indices.size(); ++i) {
        int idx = indices[i];
        cv::Rect box = boxes[idx];
        int mode = classIds[idx];
        const char* label;
        cv::Scalar color;

        switch(mode)
        {
            case 0:
                label = "Car";
                color = cv::Scalar(0, 0, 255);
                break;
            case 1:
                label = "Man";
                color = cv::Scalar(0, 255, 0);
                break;
            default:
                label = "License";
                color = cv::Scalar(255, 0, 0);
                break;
        }

        cv::rectangle(image, box, color, 2);
        cv::putText(image, label, cv::Point(box.x, box.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
    }
}
