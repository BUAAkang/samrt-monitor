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



    yoloNet = cv::dnn::readNetFromONNX("C:/Users/Administrator/PycharmProjects/pythonProject/yolov8n.onnx");

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
    float confidence_threshold = 0.8;
    float car_confidence_threshold = 0.8;

    int num_detections = detections.size[1]; // 检测数量
    // std::cout << "num_detections: [" << num_detections << "]" << std::endl;
    int dimensions = detections.size[2]; // 每个检测的维度数
    std::cout << "dimensions: [" << dimensions << "]" << std::endl;

    double image_width = image.cols; // 图像宽度
    double image_height = image.rows; // 图像高度
    // std::cout << "detections: [" << detections.ptr<float>(0, 5)[0] << "]" << std::endl;
    // std::cout << "detections: [" << detections.ptr<float>(0, 6)[0] << "]" << std::endl;
    // std::cout << "detections: [" << detections.ptr<float>(0, 7)[0] << "]" << std::endl;
    // std::cout << "detections: [" << detections.ptr<float>(0, 8)[0] << "]" << std::endl;
    // std::cout << "detections: [" << detections.ptr<float>(0, 9)[0] << "]" << std::endl;
    // std::cout << "Image size: [" << image.cols << " x " << image.rows << "]" << std::endl;

    for (int i = 0; i < 8400; ++i) {
        // 解析检测框的值
        float confidence=detections.ptr<float>(0, 4)[i];
        float car_confidence = detections.ptr<float>(0, 6)[i];

        bool is_person = confidence > confidence_threshold;
        bool is_car = car_confidence > car_confidence_threshold;

        if (is_person || is_car) {
            float centerX = detections.ptr<float>(0, 0)[i]*3 ; // 将归一化坐标转换为像素坐标
            float centerY = detections.ptr<float>(0, 1)[i]*float(image_height/64) ; // 将归一化坐标转换为像素坐标
            float width = detections.ptr<float>(0, 2)[i]*3 ; // 将归一化坐标转换为像素坐标
            float height = detections.ptr<float>(0, 3)[i]*float(image_height/64) ; // 将归一化坐标转换为像素坐标

            float left = centerX - width / 2; // 计算左上角X坐标
            float top = centerY - height / 2; // 计算左上角Y坐标

            // 打印边界框信息用于调试
            // std::cout << "centerX: " << centerX << " centerY: " << centerY << " width: " << width << " height: " << height << " confidence: " << confidence << " left: " << left << " top: " << top << std::endl;

            // 定义边界框
            cv::Rect rect(left, top, width, height);
            std::string label;
            if (is_person) {
                label = "Person";
                cv::rectangle(image, rect, cv::Scalar(0, 0, 255), 2);
                cv::putText(image, label, cv::Point(left, top - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
            } else if (is_car) {
                label = "Car";
                cv::rectangle(image, rect, cv::Scalar(0, 255, 0), 2);
                cv::putText(image, label, cv::Point(left, top - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
            }
            // std::cout << "Error here!" << std::endl;
        }
    }
}
