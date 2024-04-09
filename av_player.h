#ifndef AV_PLAYER_H
#define AV_PLAYER_H
#include "FFmpegInclude.h"
#include "av_decoder.h"
#include "qsharedpointer.h"
#include <QSize>
#include "av_clock.h"
#include <QAudioOutput>
#include <mutex>
#include "Vframe.h"
using namespace AVDecoder;
class YUV420Frame;
typedef Decoder::MyFrame MyFrame;
class AVPlayer : public QObject
{
    Q_OBJECT
    // Q_PROPERTY(PlayState playstate READ playstate NOTIFY playStatedChanged)
public:
    enum PlayState
    {
        AV_STOPED,
        AV_PLAYING,
        AV_PAUSED
    };
    AVPlayer();
    ~AVPlayer();
    PlayState playstate() const;
    void setVFrameSize(const QSize &size);
    inline void seekTo(int32_t time_s)
    {
        if (time_s < 0)
            time_s = 0;
        m_pDecoder->seekTo(time_s, time_s - (int32_t)m_audioClock.getClock());
    }
    inline void seekBy(int32_t time_s)
    {
        seekTo((int32_t)m_audioClock.getClock() + time_s);
    }

    int play(QString Url);
    void pause(bool isPause);
    AVPlayer::PlayState getPlayState();
    int initVideo();
    int initAudio();
    void clearPlayer();
    void initAVClock();
    void displayImage(AVFrame *frame);
    void videoCallBack(std::shared_ptr<void> par);
    // void audioCallBack(void *userData, uint8_t *stream, int len);
    double computerTargetDelay(double delay);
    static AVPlayer *GetPlay();
    double vpDuration(MyFrame *lastFrame, MyFrame *curFrame);
    double computeTargetPlay(double delay);
    void setVolume(int volume);
    void setSpeed(float speed);
signals:
    void frameChanged(QSharedPointer<YUV420Frame> frame);
    void AVTerminate();
    void AVPtsChanged(unsigned int pts);
    void AVDurationChanged(unsigned int duration);
    void playStatedChanged(AVPlayer::PlayState state);

public:
    mutex m_mutex;
    // 解码器实例
    Decoder *m_pDecoder;
    AVFormatContext *m_pFormatContext;

    AVCodecParameters *m_audioCodecPar;
    SwrContext *m_swrCtx;
    uint8_t *m_audiobuffer;

    uint32_t m_duration;
    uint32_t m_audioBufSize;
    uint32_t m_audioBufIndex; // 音频缓存索引

    uint32_t m_lastAudioPts;
    enum AVSampleFormat m_audioSampleFmt;

    // 终止标志
    int m_exit;
    // 记录暂停前的时间
    double m_PauseTime;
    // 暂停标志
    int m_pause;

    QAudioOutput *output = NULL;
    QIODevice *io = NULL;

    // 记录音视频最新的播放帧的时间戳，用于同步
    double m_frameTimer;

    // 延时时间
    double m_delay;
    // 播放速度
    float m_speed;
    // 速度设置
    bool isSetSpeed;
    // 音频播放时钟
    AVClock m_audioClock;
    // 视频播放时钟
    AVClock m_vedioClock;
    int m_volume;
    /// 目标声道数
    int m_targetChannels;
    /// 目标频率
    int m_targetFreq;
    /// 目标声道布局
    int m_targetChannelLayout;
    /// 目标样本数
    int m_targetNbSameples;
    // 目标格式
    enum AVSampleFormat m_targetSamplefmt;

    // 同步时钟初始化标志，音频异步线程
    // 谁先读到初始化标志就由社初始化时钟

    // 初始化标志
    int m_clockInitFlag;

    // 音频和视频索引
    int m_audioIndex;
    int m_videoIndex;

    // 图像宽度
    int m_imageWidth;
    int m_imamgeHeight;

    // 音频帧
    AVFrame *m_audioFrame;

    // 视频编码参数
    AVCodecParameters *m_videoCodecPar;

    // 目标像素格式
    enum AVPixelFormat m_dstPixFmt;
    int m_swsFlags;
    SwsContext *m_swsCtx;

    // 缓冲区
    uint8_t *m_buffer;

    // 像素缓冲区
    uint8_t *m_pixels[4];
    int m_pitch[4];
};
Q_DECLARE_METATYPE(AVPlayer::PlayState)

#endif