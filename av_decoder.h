#ifndef AV_CODEDER_H
#define AV_CODEDER_H

#include "FFmpegInclude.h"
#include "mutex"
#include "condition_variable"
#include <vector>
#include "vedioFFmpeg.h"
#include "audioFFmpeg.h"
#include "threadpool.h"
QT_BEGIN_NAMESPACE
namespace AVDecoder
{
    class Decoder
    {

    public:
        typedef struct MyFrame
        {
            AVFrame frame;
            int serial;
            double duration;
            double pts;
        } MyFrame;

        typedef struct MyPacket
        {
            AVPacket packet;
            int serial;
        } MyPacket;

        typedef struct PacketQueue
        {
            // 存储包的向量
            QVector<MyPacket> pktVec;
            // 读取索引
            int readIndex;
            // 写入索引
            int pushIndex;
            // 包的数量
            int size;
            // 序列号
            int serial;
            // 互斥锁
            std::mutex mutex;
            // 条件变量
            std::condition_variable cond;
        } PacketQueue;

        typedef struct FrameQueue
        {
            // 帧向量
            QVector<MyFrame> frameVec;
            // 读取索引
            int readIndex;
            // 添加索引
            int pushIndex;
            // 显示计数
            int shown;
            // 队列大小
            int size;
            // 互斥锁
            std::mutex mutex;
            // 条件变量
            std::condition_variable cond;
        } FrameQueue;

        typedef struct PktDecoder
        {
            AVCodecContext *codecCtx;
            int serial;
        } PktDecoder;

    public:
        Decoder();
        ~Decoder();

        bool init();
        int getAFrame(AVFrame *frame);
        static Decoder *getDecoder()
        {
            static Decoder *decoder = new Decoder();
            return decoder;
        }
        int getRemainingVFrame();
        // 查看上一帧
        MyFrame *peekLastVframe();
        // 查看当前帧
        MyFrame *peekVframe();
        // 查看下一帧
        MyFrame *peekNextVFrame();
        // 将读索引后移一位
        void setNextVFrame();

        inline int vidPktSerial() const
        {
            return m_vedioPacketQueue.serial;
        }

        inline int audioIndex() const
        {
            return m_audioIndex;
        }

        inline int videoIndex() const
        {
            return m_videoIndex;
        }
        inline uint32_t avDuration()
        {
            return m_duration;
        }

        inline int isExit()
        {
            return m_exit;
        }
        inline AVCodecParameters *audioCodecPar() const
        {
            return m_fmtCtx->streams[m_audioIndex]->codecpar;
        }
        inline AVCodecParameters *vedioCodercPar() const
        {
            return m_fmtCtx->streams[m_videoIndex]->codecpar;
        }
        void decode(const QString &url);

        void exit();
        void clearQueueCache();
        void demux(std::shared_ptr<void> par);
        void audioDecoder(std::shared_ptr<void> par);
        void vedioDecoder(std::shared_ptr<void> par);
        void seekTo(int32_t target, int32_t seekRel);
        void seeKBy(int32_t time_s);

    public:
        PacketQueue m_audioPacketQueue;
        PacketQueue m_vedioPacketQueue;

        FrameQueue m_audioFrameQueue;
        FrameQueue m_vedioFrameQueue;

        const int m_maxFrameQueueSize;
        const int m_maxPacketQueueSize;

        AVRational m_vidFrameRate;
        int m_audioIndex;
        int m_videoIndex;

        AVFormatContext *m_fmtCtx;

        PktDecoder m_audioPktDecoder;
        PktDecoder m_videoPktDecoder;

        uint32_t m_duration;
        char m_errBuf[100];

        // 跳转相关变量
        // 是否执行
        int m_isSeek;
        // 跳转后等待目标帧标志
        int m_viSeek;
        int m_audSeek;
        // 跳转相对
        int64_t m_seekRel;
        // 跳转绝对
        int64_t m_seekTarget;

        int m_exit;

    public:
        void setInitVal();
        void packetQueueFlush(PacketQueue *queue);
        void pushPacket(PacketQueue *queue, AVPacket *pkt);
        void pushAFrame(AVFrame *frame);
        void pushVFrame(AVFrame *frame);
        int getPacket(PacketQueue *queue, AVPacket *pkt, PktDecoder *decoder);
    };
}
#endif AV_CODEDER_H