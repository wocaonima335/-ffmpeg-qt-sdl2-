#include "av_decoder.h"
#include <thread>
#include <QThread>
AVDecoder::Decoder::Decoder() : m_maxFrameQueueSize(16), m_maxPacketQueueSize(30), m_audioIndex(-1), m_videoIndex(-1)
{
}

AVDecoder::Decoder::~Decoder()
{
    exit();
}

bool AVDecoder::Decoder::init()
{
    if (!ThreadPool::init())
    {
        qDebug() << "threadpool init failed!\n";
        return false;
    }
    m_audioPacketQueue.pktVec.resize(m_maxPacketQueueSize);
    m_vedioPacketQueue.pktVec.resize(m_maxPacketQueueSize);

    m_audioFrameQueue.frameVec.resize(m_maxFrameQueueSize);
    m_vedioFrameQueue.frameVec.resize(m_maxFrameQueueSize);

    m_audioPktDecoder.codecCtx = nullptr;
    m_videoPktDecoder.codecCtx = nullptr;

    return true;
}

int AVDecoder::Decoder::getAFrame(AVFrame *frame)
{
    if (!frame)
    {
        return 0;
    }
    std::unique_lock<std::mutex> lock(m_audioFrameQueue.mutex);
    while (!m_audioFrameQueue.size)
    {
        bool ret = m_audioFrameQueue.cond.wait_for(lock, std::chrono::milliseconds(100), [&]()
                                                   { return m_audioFrameQueue.size& !m_exit;; });
        if (!ret)
        {
            return 0;
        }
    }
    if (m_audioFrameQueue.frameVec[m_audioFrameQueue.readIndex].serial != m_audioPacketQueue.serial)
    {
        av_frame_unref(&m_audioFrameQueue.frameVec[m_audioFrameQueue.readIndex].frame);
        m_audioFrameQueue.readIndex = (m_audioIndex + 1) % m_maxFrameQueueSize;
        m_audioFrameQueue.size--;
        return 0;
    }
    av_frame_ref(frame, &m_audioFrameQueue.frameVec[m_audioFrameQueue.readIndex].frame);
    m_audioFrameQueue.readIndex = (m_audioFrameQueue.readIndex + 1) % m_maxFrameQueueSize;
    // m_audioFrameQueue.readIndex++;
    m_audioFrameQueue.size--;
    // if (m_audioFrameQueue.size <= 0)
    // {
    //     return 0;
    // }
    return 1;
}

int AVDecoder::Decoder::getRemainingVFrame()
{
    // 检查视频帧队列是否为空
    if (!m_vedioFrameQueue.size)
        return 0;
    // 返回未显示的帧数
    return m_vedioFrameQueue.size - m_vedioFrameQueue.shown;
    // 返回0
}

AVDecoder::Decoder::MyFrame *AVDecoder::Decoder::peekLastVframe()
{

    Decoder::MyFrame *frame = &m_vedioFrameQueue.frameVec[m_vedioFrameQueue.readIndex];
    // if (m_vedioFrameQueue.frameVec[m_vedioFrameQueue.readIndex].frame.format != 0)
    // {
    //     printf("error of one frame!\n");
    // }
    return frame;
}

AVDecoder::Decoder::MyFrame *AVDecoder::Decoder::peekVframe()
{
    // 如果没有帧，则等待100毫秒
    while (!m_vedioFrameQueue.size)
    {
        std::unique_lock<std::mutex> lock(m_vedioFrameQueue.mutex);
        bool ret = m_vedioFrameQueue.cond.wait_for(lock, std::chrono::milliseconds(100), [this]
                                                   { return m_vedioFrameQueue.size& !m_exit;; });
        if (!ret)
        {
            return nullptr;
        }
    }
    // 从队列中获取帧
    int index = (m_vedioFrameQueue.readIndex + m_vedioFrameQueue.shown) % m_maxFrameQueueSize;
    // if (m_vedioFrameQueue.frameVec[index].frame.format != 0)
    // {
    //     printf("error of two frame!\n");
    // }
    Decoder::MyFrame *frame = &m_vedioFrameQueue.frameVec[index];
    return frame;
}
AVDecoder::Decoder::MyFrame *AVDecoder::Decoder::peekNextVFrame()
{

    while (m_vedioFrameQueue.size < 2)
    {
        unique_lock<std::mutex> lock(m_vedioFrameQueue.mutex);
        bool ret = m_vedioFrameQueue.cond.wait_for(lock, std::chrono::milliseconds(100), [this]()
                                                   { return m_vedioFrameQueue.size & !m_exit; });
        if (!ret)
        {
            return nullptr;
        }
    }
    int index = (m_vedioFrameQueue.readIndex + m_vedioFrameQueue.shown + 1) % m_maxFrameQueueSize;
    // if (m_vedioFrameQueue.frameVec[index].frame.format != 0)
    // {
    //     printf("error of three frame! %d\n", m_vedioFrameQueue.readIndex);
    // }
    Decoder::MyFrame *frame = &m_vedioFrameQueue.frameVec[index];
    return frame;
}
void AVDecoder::Decoder::setNextVFrame()
{
    std::unique_lock<std::mutex> lock(m_vedioFrameQueue.mutex);
    if (!m_vedioFrameQueue.size)
    {
        return;
    }
    // 如果队列中的帧未被显示，则将其设置为已显示，并返回
    if (!m_vedioFrameQueue.shown)
    {
        m_vedioFrameQueue.shown = 1;
        return;
    }
    // 将当前读取到的帧reference释放，并将下一个帧设置为当前帧
    av_frame_unref(&m_vedioFrameQueue.frameVec[m_vedioFrameQueue.readIndex].frame);
    m_vedioFrameQueue.readIndex = (m_vedioFrameQueue.readIndex + 1) % m_maxFrameQueueSize;
    if (m_vedioFrameQueue.frameVec[m_vedioFrameQueue.readIndex].frame.format != 0)
    {
        printf("error of this frame!,%d\n", m_vedioFrameQueue.readIndex + 1);
    }
    m_vedioFrameQueue.size--;
}
void AVDecoder::Decoder::decode(const QString &url)
{
    // 用于获取流时长
    AVDictionary *formatOpts = nullptr;
    av_dict_set(&formatOpts, "probesize", "32", 0);

    // 打开流
    printf("path:%s\n", url.toStdString().c_str());
    bool ret = VedioFFmpeg::Get()->Open(url.toStdString().c_str());
    if (ret < 0)
    {
        printf("open Vedio url failed\n");
    }
    ret = Audio::AudioFFmpeg::GetAudio()->open(url.toStdString().c_str());
    if (ret < 0)
    {

        printf("open Audio url failed\n");
    }
    m_fmtCtx = Audio::AudioFFmpeg::GetAudio()->Aavformacontext;
    m_videoIndex = VedioFFmpeg::Get()->VedioStream;
    m_audioIndex = Audio::AudioFFmpeg::GetAudio()->audioStreamIndex;
    m_audioPktDecoder.codecCtx = Audio::AudioFFmpeg::GetAudio()->Aacodeccontext;
    m_videoPktDecoder.codecCtx = VedioFFmpeg::Get()->Vcodecctx;
    // 记录流时长
    AVRational q = {1, AV_TIME_BASE};
    m_duration = (uint32_t)(m_fmtCtx->duration * av_q2d(q));
    // 记录视频帧率
    m_vidFrameRate = av_guess_frame_rate(m_fmtCtx, m_fmtCtx->streams[m_videoIndex], nullptr);
    setInitVal();
    ThreadPool::addTask(std::bind(&Decoder::demux, this, std::placeholders::_1), std::make_shared<int>(1));
    ThreadPool::addTask(std::bind(&Decoder::audioDecoder, this, std::placeholders::_1), std::make_shared<int>(2));
    ThreadPool::addTask(std::bind(&Decoder::vedioDecoder, this, std::placeholders::_1), std::make_shared<int>(3));

    printf("open Audio url success\n");
}

void AVDecoder::Decoder::exit()
{
    m_exit = 1;
    QThread::msleep(200);
    clearQueueCache();
    if (m_fmtCtx)
    {
        VedioFFmpeg::Get()->Close();
        Audio::AudioFFmpeg::GetAudio()->close();
        m_fmtCtx = nullptr;
    };
}

void AVDecoder::Decoder::clearQueueCache()
{
    std::lock_guard<std::mutex> lockAP(m_audioPacketQueue.mutex);
    std::lock_guard<std::mutex> lockVP(m_vedioPacketQueue.mutex);
    while (m_audioPacketQueue.size)
    {
        av_packet_unref(&m_audioPacketQueue.pktVec[m_audioPacketQueue.readIndex].packet);
        m_audioPacketQueue.readIndex = (m_audioPacketQueue.readIndex + 1) % m_maxPacketQueueSize;
        m_audioPacketQueue.size--;
    }
    while (m_vedioPacketQueue.size)
    {
        av_packet_unref(&m_vedioPacketQueue.pktVec[m_vedioPacketQueue.readIndex].packet);
        m_vedioPacketQueue.readIndex = (m_vedioPacketQueue.readIndex + 1) % m_maxPacketQueueSize;
        m_vedioPacketQueue.size--;
    }
    std::lock_guard<std::mutex> lockAF(m_audioFrameQueue.mutex);
    std::lock_guard<std::mutex> lockVF(m_vedioFrameQueue.mutex);
    while (m_audioFrameQueue.size)
    {
        av_frame_unref(&m_audioFrameQueue.frameVec[m_audioFrameQueue.readIndex].frame);
        m_audioFrameQueue.readIndex = (m_audioFrameQueue.readIndex + 1) % m_maxFrameQueueSize;
        m_audioFrameQueue.size--;
    }

    while (m_vedioFrameQueue.size)
    {
        av_frame_unref(&m_vedioFrameQueue.frameVec[m_vedioFrameQueue.readIndex].frame);
        m_vedioFrameQueue.readIndex = (m_vedioFrameQueue.readIndex + 1) % m_maxFrameQueueSize;
        m_vedioFrameQueue.size--;
    }
}

void AVDecoder::Decoder::demux(std::shared_ptr<void> par)
{
    AVPacket *AVpkt = av_packet_alloc();
    int ret = -1;
    int num = 0;
    for (;;)
    {

        // printf("nums=%d\n", num++);
        //|| m_vedioPacketQueue.size >= m_maxPacketQueueSize
        if (m_audioPacketQueue.size >= m_maxPacketQueueSize || m_vedioPacketQueue.size >= m_maxPacketQueueSize)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (m_isSeek)
        {
            int64_t seekTarget = m_seekTarget * AV_TIME_BASE;
            ret = av_seek_frame(m_fmtCtx, -1, seekTarget, AVSEEK_FLAG_BACKWARD);
            if (ret < 0)
            {
                printf("av_seek_frame error\n");
                return;
            }
            else
            {
                packetQueueFlush(&m_audioPacketQueue);
                packetQueueFlush(&m_vedioPacketQueue);
                m_audSeek = 1;
                m_viSeek = 1;
            }
            m_isSeek = 0;
        }
        AVpkt = Audio::AudioFFmpeg::GetAudio()->Read();
        if (AVpkt == nullptr)
        {
            break;
        }
        if (AVpkt->stream_index == Audio::AudioFFmpeg::GetAudio()->audioStreamIndex)
        {
            // 插入音频包队列
            pushPacket(&m_audioPacketQueue, AVpkt);
        }
        else if (AVpkt->stream_index == VedioFFmpeg::Get()->VedioStream)
        {
            // 插入视频包队列
            pushPacket(&m_vedioPacketQueue, AVpkt);
        }
        else
        {
            av_packet_unref(AVpkt);
        }

        // av_usleep(50);
    }
    // 释放pkt
    av_packet_free(&AVpkt);
    if (!m_exit)
    {
        while (m_audioFrameQueue.size)
            QThread::msleep(50);
        exit();
        printf("demux of thread exit\n");
    }
    // printf(" video  queue of len:%d\n", AVDecoder::Decoder::getDecoder()->m_vedioPacketQueue.size);
    // printf(" audio  queue of len:%d\n", AVDecoder::Decoder::getDecoder()->m_audioPacketQueue.size);
}

void AVDecoder::Decoder::setInitVal()
{
    m_audioPacketQueue.size = 0;
    m_audioPacketQueue.pushIndex = 0;
    m_audioPacketQueue.readIndex = 0;
    m_audioPacketQueue.serial = 0;

    m_vedioPacketQueue.size = 0;
    m_vedioPacketQueue.pushIndex = 0;
    m_vedioPacketQueue.readIndex = 0;
    m_vedioPacketQueue.serial = 0;

    m_audioFrameQueue.size = 0;
    m_audioFrameQueue.readIndex = 0;
    m_audioFrameQueue.pushIndex = 0;
    m_audioFrameQueue.shown = 0;

    m_vedioFrameQueue.size = 0;
    m_vedioFrameQueue.readIndex = 0;
    m_vedioFrameQueue.pushIndex = 0;
    m_vedioFrameQueue.shown = 0;

    m_audioPktDecoder.serial = 0;
    m_videoPktDecoder.serial = 0;

    m_exit = 0;

    m_isSeek = 0;
    m_audSeek = 0;
    m_viSeek = 0;
}

void AVDecoder::Decoder::packetQueueFlush(PacketQueue *queue)
{
    std::lock_guard<std::mutex> lockAP(queue->mutex);
    // 清空队列
    while (queue->size)
    {
        // 释放队列中的包
        av_packet_unref(&queue->pktVec[queue->readIndex].packet);
        // 读取下一个包
        queue->readIndex = (queue->readIndex + 1) % m_maxPacketQueueSize;
        // 队列大小减一
        queue->size--;
    }
    queue->serial++;
}

void AVDecoder::Decoder::pushPacket(PacketQueue *queue, AVPacket *pkt)
{
    // 加锁，保护队列
    std::lock_guard<std::mutex> lock(queue->mutex);
    // 将pkt的引用移动到队列的pktVec中，并设置序列号
    av_packet_move_ref(&queue->pktVec[queue->pushIndex].packet, pkt);
    queue->pktVec[queue->pushIndex].serial = queue->serial;
    if (queue->serial == 1)
    {
        printf(" ");
    }
    // 将pushIndex+1,取余m_maxPacketQueueSize,得到新的pushIndex;
    queue->pushIndex = (queue->pushIndex + 1) % m_maxPacketQueueSize;
    // queue->pushIndex++;
    //  size+1
    queue->size++;
}

void AVDecoder::Decoder::pushAFrame(AVFrame *frame)
{
    // 加锁，保护队列
    std::lock_guard<std::mutex> lock(m_audioFrameQueue.mutex);
    // 将frame移动到队列中
    av_frame_move_ref(&m_audioFrameQueue.frameVec[m_audioFrameQueue.pushIndex].frame, frame);
    // 设置序列号
    m_audioFrameQueue.frameVec[m_audioFrameQueue.pushIndex].serial = m_audioPktDecoder.serial;
    // 更新索引
    m_audioFrameQueue.pushIndex = (m_audioFrameQueue.pushIndex + 1) % m_maxFrameQueueSize;
    // // 设置音频帧的pts
    m_audioFrameQueue.frameVec[m_audioFrameQueue.pushIndex].pts =
        m_audioFrameQueue.frameVec[m_audioFrameQueue.pushIndex].frame.pts * av_q2d(m_fmtCtx->streams[m_audioIndex]->time_base);
    // m_audioFrameQueue.pushIndex++;
    // 队列长度加1
    m_audioFrameQueue.size++;
}

void AVDecoder::Decoder::pushVFrame(AVFrame *frame)
{
    std::lock_guard<std::mutex> lock(m_vedioPacketQueue.mutex);
    if (frame->format != 0)
    {
        printf("error of frame!\n");
    }
    av_frame_move_ref(&m_vedioFrameQueue.frameVec[m_vedioFrameQueue.pushIndex].frame, frame);

    //  printf("error of decode frame! %d\n", m_vedioFrameQueue.frameVec[m_vedioFrameQueue.pushIndex].frame.format);

    m_vedioFrameQueue.frameVec[m_vedioFrameQueue.pushIndex].serial = m_videoPktDecoder.serial;
    // 持续时间和显示时间戳
    // 设置视频帧的持续时间
    m_vedioFrameQueue.frameVec[m_vedioFrameQueue.pushIndex].duration = (m_vidFrameRate.den && m_vidFrameRate.den) ? av_q2d(AVRational{m_vidFrameRate.den, m_vidFrameRate.num}) : 0.00;
    // // 设置视频帧的pts
    m_vedioFrameQueue.frameVec[m_vedioFrameQueue.pushIndex].pts =
        m_vedioFrameQueue.frameVec[m_vedioFrameQueue.pushIndex].frame.pts * av_q2d(m_fmtCtx->streams[m_videoIndex]->time_base);
    m_vedioFrameQueue.pushIndex = (m_vedioFrameQueue.pushIndex + 1) % m_maxFrameQueueSize;
    // m_vedioFrameQueue.pushIndex++;
    m_vedioFrameQueue.size++;
}

void AVDecoder::Decoder::audioDecoder(std::shared_ptr<void> par)
{
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    int errtimes = 10;
    for (;;)
    {
        if (m_exit)
        {
            break;
        }
        // 音频帧队列长度控制
        if (m_audioFrameQueue.size >= m_maxFrameQueueSize)
        {
            this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        // 从音频包队列取音频包
        int ret = getPacket(&m_audioPacketQueue, pkt, &m_audioPktDecoder);
        if (ret)
        {
            frame = Audio::AudioFFmpeg::GetAudio()->Decode(pkt);
            if (frame == nullptr)
            {
                if (errtimes == 0)
                {

                    printf("Decode audio packet failed!!!\n ");
                    break;
                }
                errtimes--;
                printf("times--\n ");
                continue;
            }
            if (frame != nullptr)
            {
                if (m_audSeek)
                {
                    int pts = (int)frame->pts * av_q2d(m_fmtCtx->streams[m_audioIndex]->time_base);
                    if (pts < m_seekTarget)
                    {
                        av_frame_unref(frame);
                        continue;
                    }
                    else
                    {
                        m_audSeek = 0;
                    }
                }
            }
            pushAFrame(frame);
        }
        else
        {
            // printf("wait audio frame\n");
            //  printf(" frame audio queue of len:%d\n", AVDecoder::Decoder::getDecoder()->m_audioFrameQueue.size);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    // printf(" frame audio queue of len:%d\n", AVDecoder::Decoder::getDecoder()->m_audioFrameQueue.size);
    av_packet_free(&pkt);
    av_frame_free(&frame);
    printf("audioDecode exit\n");
}

void AVDecoder::Decoder::vedioDecoder(std::shared_ptr<void> par)
{
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    for (;;)
    {
        if (m_exit)
        {
            break;
        }
        if (m_vedioFrameQueue.size >= m_maxFrameQueueSize)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        int ret = getPacket(&m_vedioPacketQueue, pkt, &m_videoPktDecoder);
        if (ret)
        {
            frame = VedioFFmpeg::Get()->Decode(pkt);
            if (frame == nullptr || frame->format != 0)
            {

                printf("decode vedio frame failed\n");
                av_frame_unref(frame);
                continue;
                // break;
            }
            if (frame != nullptr)
            {
                if (m_viSeek)
                {
                    int pts = (int)frame->pts * av_q2d(m_fmtCtx->streams[m_videoIndex]->time_base);
                    if (pts < m_seekTarget)
                    {
                        av_frame_unref(frame);
                        continue;
                    }
                    else
                    {
                        m_viSeek = 0;
                    }
                }
                pushVFrame(frame);
            }
        }
        else
        {
            // printf("wait vedio frame\n");
            //  printf("frame vedio queue of len: %d\n", AVDecoder::Decoder::getDecoder()->m_vedioFrameQueue.size);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    av_packet_free(&pkt);
    av_frame_free(&frame);
    printf("VedioDecode exit\n");
}

void AVDecoder::Decoder::seekTo(int32_t target, int32_t seekRel)
{
    // 上次跳转未完成不处理跳转请求
    if (m_isSeek == 1)
        return;
    if (target < 0)
        target = 0;
    m_seekTarget = target;
    m_seekRel = seekRel;
    m_isSeek = 1;
}

int AVDecoder::Decoder::getPacket(PacketQueue *queue, AVPacket *pkt, PktDecoder *decoder)
{
    std::unique_lock<std::mutex> lock(queue->mutex);
    // 如果队列大小为0，则等待，直到队列大小不为0
    while (!queue->size)
    {
        // 使用条件变量等待，每100毫秒检查一次队列大小
        bool ret = queue->cond.wait_for(lock, std::chrono::milliseconds(100), [&]()
                                        { return queue->size & !m_exit; });
        // 如果等待超时，则返回0
        if (!ret)
        {
            return 0;
        }
    }
    if (queue->serial != decoder->serial)
    // 序号不连续的帧说明发生了跳转操作直接丢弃
    // 并情空解码器缓存
    {
        avcodec_flush_buffers(decoder->codecCtx);
        decoder->serial = queue->pktVec[queue->readIndex].serial;
        return 0;
    } // 返回队列大小
    av_packet_move_ref(pkt, &queue->pktVec[queue->readIndex].packet);
    // 设置解码器序列号
    decoder->serial = queue->pktVec[queue->readIndex].serial;
    // 读取索引加一，取余数
    queue->readIndex = (queue->readIndex + 1) % m_maxPacketQueueSize;
    // queue->readIndex++;
    //  队列大小减一
    queue->size--;
    return 1;
}
