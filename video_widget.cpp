#include "video_widget.h"
#include <QPainter>
#include <QDebug>

VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent),
    timer(new QTimer(this)),
    formatContext(nullptr),
    codecContext(nullptr),
    frame(av_frame_alloc()),
    packet(av_packet_alloc()),
    swsContext(nullptr),
    videoStreamIndex(-1)
{
    av_log_set_level(AV_LOG_INFO);
    avformat_network_init();

    // 打开视频流
    const char *videoUrl = "rtsp://192.168.1.26:554/stream0"; // 替换为实际IP地址
    if (avformat_open_input(&formatContext, videoUrl, nullptr, nullptr) != 0) {
        qDebug() << "Could not open input";
        return;
    }

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
                                codecContext->width, codecContext->height, AV_PIX_FMT_RGB24,
                                SWS_BILINEAR, nullptr, nullptr, nullptr);

    connect(timer, &QTimer::timeout, this, &VideoWidget::onFrameDecoded);
    timer->start(33); // 大约30帧每秒
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
    return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888).rgbSwapped();
}
