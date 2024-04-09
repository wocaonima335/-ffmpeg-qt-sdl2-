#include "VedioFFmpeg.h"
#include "av_player.h"
#include "opengl_widget.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include "qwidget.h"
#include "opengl_widget.h"
int YUV420Frame::num = 0;
VedioFFmpeg::VedioFFmpeg()
{
    errorbuffer[1024] = '\0';
    vediobuffer[1024] = '\0';
    av_register_all();       // 注册FFmpeg的库
    avformat_network_init(); // 初始化网络
}

int VedioFFmpeg::Getpts()
{
    return Vframe->pts;
}

VedioFFmpeg *VedioFFmpeg::Get()
{
    static VedioFFmpeg vedioffmpeg;
    return &vedioffmpeg;
}

VedioFFmpeg::~VedioFFmpeg()
{
}

bool VedioFFmpeg::Open(const char *path)
{

    vedio_frame_mutex.lock();
    int res = avformat_open_input(&Vavformtctx, path, NULL, NULL);
    if (res != 0) // 打开视频文件
    {
        vedio_frame_mutex.unlock();
        av_strerror(res, errorbuffer, 1024);
        printf("open %s failed:%s\n", path, errorbuffer);
        return false;
    }
    int totalMs = Vavformtctx->duration / (AV_TIME_BASE); // 获取视频总时间
    // 获取视频信息
    avformat_find_stream_info(Vavformtctx, NULL);
    int StreamNums = Vavformtctx->nb_streams;
    qint32 Duration = Vavformtctx->duration / 1000000; // 1秒=1000000微秒
    qint32 StartTime = Vavformtctx->start_time;
    qint64 Bite = Vavformtctx->bit_rate;
    string buffer = std::to_string(totalMs) + " StreamNums:" + std::to_string(StreamNums) + " Durations:" + std::to_string(Duration) + " Start:" + std::to_string(StartTime) + " Bite:" + std::to_string(Bite);
    strcpy(vediobuffer, buffer.c_str());
    printf("open %s success, totalMs:%d\n", path, totalMs);
    // 寻找解码器
    for (int i = 0; i < Vavformtctx->nb_streams; i++)
    {
        Vcodecctx = Vavformtctx->streams[i]->codec;
        if (Vcodecctx->codec_type == AVMEDIA_TYPE_VIDEO) // 判断是否为视频流
        {
            VedioStream = i;                                                            // 视频流索引
            Vcodec = avcodec_find_decoder(Vavformtctx->streams[i]->codecpar->codec_id); // 查找解码器
            if (!Vcodec)
            {
                vedio_frame_mutex.unlock();
                printf("vedio codec no found\n");
                return false;
            }
            av_opt_set(Vcodecctx->priv_data, "tune", "zerolatency", 0);
            Vcodecctx->codec_id = AV_CODEC_ID_H264;
            int err = avcodec_open2(Vcodecctx, Vcodec, nullptr); // 打开解码器
            if (err != 0)
            {
                vedio_frame_mutex.unlock();
                char buf[1024] = {0};
                av_strerror(err, errorbuffer, sizeof(errorbuffer));
                printf(buf);
                return false;
            }
            break;
        }
        if (VedioStream == -1)
        {
            vedio_frame_mutex.unlock();
            printf("not find audio stream\n");
            return false;
        }
    }
    vedio_frame_mutex.unlock();
    return true;
}

void VedioFFmpeg::Close()
{
    vedio_frame_mutex.lock();
    if (Vavformtctx != NULL)
    {
        avformat_close_input(&Vavformtctx);
        Vavformtctx = NULL;
        Vcodecctx = NULL;
        Vcodec = NULL;
    }

    if (Vframe != nullptr)
    {
        av_frame_free(&Vframe);
        Vframe = nullptr;
    }
    if (Vswsctx)
    {
        sws_freeContext(Vswsctx);
        Vswsctx = nullptr;
    }
    vedio_frame_mutex.unlock();
}

string VedioFFmpeg::GetError()
{

    return string(errorbuffer);
}

AVPacket *VedioFFmpeg::Read()
{
    memset(&Vpacket, 0, sizeof(Vpacket));
    vedio_frame_mutex.lock();
    if (!Vavformtctx)
    {
        vedio_frame_mutex.unlock();
        return nullptr;
    }
    int err = av_read_frame(Vavformtctx, &Vpacket);
    if (err != 0) // 读取失败
    {
        av_strerror(err, errorbuffer, sizeof(errorbuffer));
        printf("read frame failed:%s\n", errorbuffer);
        vedio_frame_mutex.unlock();
        return nullptr;
    }
    vedio_frame_mutex.unlock();
    return &Vpacket;
}

AVFrame *VedioFFmpeg::Decode(const AVPacket *temp)
{
    vedio_frame_mutex.lock();
    if (!Vavformtctx)
    {
        vedio_frame_mutex.unlock();
        return nullptr;
    }
    if (Vframe == nullptr)
    {
        Vframe = av_frame_alloc();
    }

    int res = avcodec_send_packet(Vcodecctx, temp);

    if (res != 0 || res == AVERROR(EAGAIN) || res == AVERROR_EOF)
    {
        vedio_frame_mutex.unlock();
        av_strerror(res, errorbuffer, sizeof(errorbuffer));
        printf("avcodec_send_packet error: %s\n", errorbuffer);
        av_packet_unref(const_cast<AVPacket *>(temp));
        return nullptr;
    }
    res = avcodec_receive_frame(Vcodecctx, Vframe); // 解码pkt后存入Vframe中
    if (res != 0 || res == AVERROR(EAGAIN) || res == AVERROR_EOF || Vframe->format != 0)
    {
        vedio_frame_mutex.unlock();
        av_strerror(res, errorbuffer, sizeof(errorbuffer));
        printf("recive decode frame failed:%s\n", errorbuffer);
        return nullptr;
    }
    vedio_frame_mutex.unlock();
    return Vframe;
}

bool VedioFFmpeg::ToRGB(const AVFrame *aframe, uint8_t *m_pixels[], int *m_ptich, int outwidth, int outheight)
{
    vedio_frame_mutex.lock();
    if (!Vavformtctx)
    {
        vedio_frame_mutex.unlock();
        return false;
    }
    // Vcodecctx =
    //     Vavformtctx->streams[this->VedioStream]->codec;
    if (aframe->format < 0)
    {
        vedio_frame_mutex.unlock();
        //printf("format is failed!!! %d and %d\n", aframe->format, aframe->pts);
        return false;
    }
    Vswsctx = sws_getCachedContext(Vswsctx, aframe->width, aframe->height, (enum AVPixelFormat)aframe->format, outwidth, outheight,
                                   AV_PIX_FMT_YUV420P, // 输出像素格式
                                   SWS_BICUBIC,        // 输出像素算法
                                   NULL, NULL, NULL);

    if (!Vswsctx)
    {
        vedio_frame_mutex.unlock();
        printf("sws_getCachedContext failed\n");
        return false;
    }
    if (Vswsctx && aframe->data && aframe->linesize && m_pixels && m_ptich)
    {
        // 调用 sws_scale 函数
        int result = sws_scale(Vswsctx, aframe->data, aframe->linesize, 0, aframe->height, m_pixels, m_ptich);
        // 检查返回值
        if (result <= 0)
        {
            // 记录错误日志或进行适当的错误处理
            fprintf(stderr, "sws_scale failed: %d\n", result);
            // 可以根据具体情况采取其他错误处理措施
        }
    }
    else
    {
        // 参数无效的错误处理
        fprintf(stderr, "Invalid input parameters for sws_scale\n");
    }

    QSharedPointer<YUV420Frame> frame = QSharedPointer<YUV420Frame>::create(m_pixels[0], aframe->width, aframe->height);
    emit frameChanged(frame);
    //   qDebug() << "sent success!!!! " << YUV420Frame::num << frame->getBufU() << frame->getBufV() << frame->getBufY();

    vedio_frame_mutex.unlock();
    return true;
}

string VedioFFmpeg::Detail()
{
    return string(vediobuffer);
}
