#ifndef VEDIOFFMPEG_H
#define VEDIOFFMPEG_H
#include "FFmpegInclude.h"
#include "QMutex"
#include <list>
#include "Vframe.h"
#include <string>
#include <QSharedPointer>
#include <QCoreApplication>
class OpenGLWidget;
using namespace std;
class VedioFFmpeg : public QObject
{
    Q_OBJECT
public:
    int Vpts;                        // 当前播放的视频帧pts
    char errorbuffer[1024];          // 报错缓存
    char vediobuffer[1024];          // 视频信息缓存
    list<AVPacket> vedio_frame_list; // 视频压缩帧队列
    QMutex vedio_frame_mutex;        // 视频帧队列互斥锁
    AVFormatContext *Vavformtctx;    // 视频格式上下文
    AVCodecContext *Vcodecctx;       // 视频解码上下文
    AVCodec *Vcodec;                 // 视频解码器
    AVFrame *Vframe;                 // 视频帧
    AVPacket Vpacket;                // 视频压缩包
    int Vstreamindex;                // 视频流索引
    SwsContext *Vswsctx;             // 视频帧格式转换上下文
    OpenGLWidget *op;

public:
    int VedioStream; // 视频流索引
    VedioFFmpeg();
    int Getpts();              // 获取压缩视频帧的pts;
    static VedioFFmpeg *Get(); // 获取单例
    ~VedioFFmpeg();
    bool Open(const char *path);                                                                       // 打开视频文件
    void Close();                                                                                      // 关闭视频文件
    string GetError();                                                                                 // 获取错误
    AVPacket *Read();                                                                                  // 读取视频帧
    AVFrame *Decode(const AVPacket *temp);                                                             // 解码视频帧
    bool ToRGB(const AVFrame *aframe, uint8_t *m_pixels[], int *m_ptich, int outwidth, int outheight); // 转化输出格式
    string Detail();
Q_SIGNALS:
    void frameChanged(QSharedPointer<YUV420Frame> frame); // 视频帧信息
};
#endif;
