#include "av_player.h"
#include <QDebug>
#include <QThread>
#include <QImage>
#include <QAudioOutput>
#include <QTimer>
#include "av_decoder.h"
#include <SDL.h>
// 同步阈值下限
#define AV_SYNC_THRESHOLD_MIN 0.04
// 同步阈值上限
#define AV_SYNC_THRESHOLD_MAX 0.1
// 单帧视频时长阈值上限，用于适配低帧时同步，
// 帧率过低视频帧超前不适合翻倍延迟，应特殊
// 处理，这里设置上限一秒10帧
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
// 同步操作摆烂阈值上限，此时同步已无意义
#define AV_NOSYNC_THRESHOLD 10.0

#define AV_SYNC_REJUDGESHOLD 0.01

AVPlayer::AVPlayer()
    : m_pDecoder(AVDecoder::Decoder::getDecoder()),
      m_audioFrame(av_frame_alloc()),
      m_imageWidth(300),
      m_imamgeHeight(300),
      m_swrCtx(Audio::AudioFFmpeg::GetAudio()->swr_ctx),
      m_swsCtx(VedioFFmpeg::Get()->Vswsctx),
      m_audiobuffer(nullptr),
      m_buffer(nullptr),
      m_duration(0),
      m_volume(30),
      m_exit(0),
      m_pause(1),
      m_speed(1),
      isSetSpeed(0)

{
}

AVPlayer::~AVPlayer()
{
    av_frame_free(&m_audioFrame);
    clearPlayer();
    delete m_pDecoder;
    if (m_swrCtx)
    {
        swr_free(&m_swrCtx);
    }
    if (m_swsCtx)
    {
        sws_freeContext(m_swsCtx);
    }
    if (m_audiobuffer)
    {
        av_free(m_audiobuffer);
    }
    if (m_buffer)
    {
        av_free(m_buffer);
    }
}

AVPlayer::PlayState AVPlayer::playstate() const
{
    return PlayState();
}

void AVPlayer::setVFrameSize(const QSize &size)
{
    m_imageWidth = size.width();
    m_imamgeHeight = size.height();
}

int AVPlayer::play(QString Url)
{
    m_pause = 0;
    m_clockInitFlag = -1;

    clearPlayer();
    AVDecoder::Decoder::getDecoder()->init();
    AVDecoder::Decoder::getDecoder()->decode(Url);
    m_duration = m_pDecoder->avDuration();
    emit AVDurationChanged(m_duration);
    if (!initAudio())
    {
        qDebug() << "init sdl failed!";
        return 0;
    };
    initVideo();

    return 0;
}

void AVPlayer::pause(bool isPause)
{
    if (SDL_GetAudioStatus() == SDL_AUDIO_STOPPED)
    {
        return;
    }
    if (isPause)
    {
        if (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING)
        {
            SDL_PauseAudio(1);
            m_PauseTime = av_gettime_relative() / 1000000.0;
            m_pause = 1;
        }
    }
    else
    {
        if (SDL_GetAudioStatus() == SDL_AUDIO_PAUSED)
        {
            SDL_PauseAudio(0);
            m_pause = 0;
            m_frameTimer += av_gettime_relative() / 1000000.0 - m_PauseTime;
        }
    }
}

AVPlayer::PlayState AVPlayer::getPlayState()
{
    AVPlayer::PlayState state;
    switch (SDL_GetAudioStatus())
    {
    case SDL_AUDIO_PLAYING:
        state = AVPlayer::AV_PLAYING;
        emit playStatedChanged(AVPlayer::AV_PLAYING);
        break;
    case SDL_AUDIO_PAUSED:
        state = AVPlayer::AV_PAUSED;
        emit playStatedChanged(AVPlayer::AV_PAUSED);
        break;
    case SDL_AUDIO_STOPPED:
        state = AVPlayer::AV_STOPED;
        emit playStatedChanged(AVPlayer::AV_STOPED);
        break;
    default:
        break;
    }
    return state;
}

int AVPlayer::initVideo()
{
    // 初始化帧计数器
    m_frameTimer = 0.00;

    // 获取解码器参数
    m_videoCodecPar = m_pDecoder->vedioCodercPar();
    // 获取视频索引
    m_videoIndex = m_pDecoder->videoIndex();
    // 获取视频宽度
    m_imageWidth = m_videoCodecPar->width;
    // 获取视频高度
    m_imamgeHeight = m_videoCodecPar->height;

    // 设置转换后帧数据格式
    m_dstPixFmt = AV_PIX_FMT_YUV420P;
    // 设置转换算法
    m_swsFlags = SWS_BICUBIC;

    // 分配存储转换后帧数据的buffer内存
    int bufSize = av_image_get_buffer_size(m_dstPixFmt, m_imageWidth, m_imamgeHeight, 1);
    m_buffer = (uint8_t *)av_realloc(m_buffer, bufSize * sizeof(uint8_t));
    // 初始化转换后帧数据
    av_image_fill_arrays(m_pixels, m_pitch, m_buffer, m_dstPixFmt, m_imageWidth, m_imamgeHeight, 1);
    // 视频帧播放回调递插入线程池任务队列
    if (!ThreadPool::addTask(std::bind(&AVPlayer::videoCallBack, this, std::placeholders::_1), std::make_shared<int>(0)))
    {
        printf("videoCallBack add task failed!\n");
    }
    return 1;
}
// int AVPlayer::initAudio()
// {
//     m_mutex.lock();
//     m_audioBufSize = 0;
//     m_audioIndex = 0;
//     m_lastAudioPts = -1;
//     m_audioCodecPar = m_pDecoder->audioCodecPar();

//     QAudioFormat format;                                                  // 设置音频输出格式
//     format.setSampleRate(m_audioCodecPar->sample_rate);                   // 采样率
//     format.setChannelCount(2);                                            // 声道数
//     format.setSampleSize(8 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16)); // 采样位数
//     format.setCodec("audio/pcm");                                         // 音频编码格式
//     format.setByteOrder(QAudioFormat::LittleEndian);                      // 字节序
//     format.setSampleType(QAudioFormat::SignedInt);                        // 采样类型
//     // format.setSampleType(QAudioFormat::UnSignedInt);
//     output = new QAudioOutput(format);

//     io = output->start();
//     m_pFormatContext = m_pDecoder->m_fmtCtx;
//     if (!ThreadPool::addTask(std::bind(&AVPlayer::audioCallBack, this, m_audiobuffer, std::placeholders::_1), std::make_shared<int>(5)))
//     {
//         printf("audioCallBack add task failed!\n");
//     }
//     m_mutex.unlock();
//     return 1;
// }
void audioCallBack(void *userData, uint8_t *stream, int len)
{
    memset(stream, 0, len);
    AVPlayer *player = (AVPlayer *)userData;
    double audioPts = 0.00;
    int times = 3;
    while (len > 0)
    {
        if (player->m_exit)
            return;
        if (player->m_audioBufIndex >= player->m_audioBufSize)
        {
            int ret = player->m_pDecoder->getAFrame(player->m_audioFrame);
            if (ret)
            {
                player->m_audioBufIndex = 0;
                if ((player->m_targetSamplefmt != player->m_audioFrame->format || player->m_targetChannelLayout != player->m_audioFrame->channel_layout ||
                     player->m_targetFreq != player->m_audioFrame->sample_rate || player->m_targetNbSameples != player->m_audioFrame->nb_samples) &&
                    !player->m_swrCtx)
                {
                    player->m_swrCtx = swr_alloc_set_opts(nullptr, player->m_targetChannelLayout, player->m_targetSamplefmt, player->m_targetFreq, player->m_audioFrame->channel_layout, (enum AVSampleFormat)player->m_audioFrame->format, player->m_audioFrame->sample_rate, 0, nullptr);
                    if (!player->m_swrCtx || swr_init(player->m_swrCtx) < 0)
                    {
                        printf("swr_init failed\n");
                        return;
                    }
                }
                if (player->m_swrCtx)
                {
                    const uint8_t **in = (const uint8_t **)player->m_audioFrame->extended_data;
                    int out_count = (uint64_t)player->m_audioFrame->nb_samples * player->m_targetFreq / player->m_audioFrame->sample_rate + 256;
                    int out_size = av_samples_get_buffer_size(nullptr, player->m_targetChannels, player->m_audioFrame->nb_samples, player->m_targetSamplefmt, 1);
                    if (out_size < 0)
                    {
                        qDebug() << "av_samples_get_buffer_size failed";
                        return;
                    }
                    av_fast_malloc(&player->m_audiobuffer, &player->m_audioBufSize, out_size);
                    if (!player->m_audiobuffer)
                    {
                        printf("av_fast_malloc failed\n");
                        return;
                    }
                    int len2 = swr_convert(player->m_swrCtx, &player->m_audiobuffer, out_count, in, player->m_audioFrame->nb_samples);

                    if (len2 < 0)
                    {
                        printf("swr_convert failed\n");
                        return;
                    }
                    player->m_audioBufSize = av_samples_get_buffer_size(nullptr, player->m_targetChannels, len2, player->m_targetSamplefmt, 0);
                }
                else
                {
                    player->m_audioBufSize = av_samples_get_buffer_size(nullptr, player->m_targetChannels, player->m_audioFrame->nb_samples, player->m_targetSamplefmt, 0);
                    av_fast_malloc(&player->m_audiobuffer, &player->m_audioBufSize, player->m_audioBufSize + 256);
                    if (!player->m_audiobuffer)
                    {
                        printf("av_fast_malloc failed\n");
                        return;
                    }
                    memcpy(player->m_audiobuffer, player->m_audioFrame->data[0], player->m_audioBufSize);
                }
                audioPts = player->m_audioFrame->pts * av_q2d(player->m_pFormatContext->streams[player->m_audioIndex]->time_base);

                av_frame_unref(player->m_audioFrame);
            }
            else
            {
                if (player->m_pDecoder->isExit())
                {
                    emit player->AVTerminate();
                    return;
                }
                if (times <= 0)
                {
                    return;
                }
                continue;
            }
        }

        int len1 = player->m_audioBufSize - player->m_audioBufIndex;
        len1 = (len1 > len ? len : len1);
        SDL_MixAudio(stream, player->m_audiobuffer + player->m_audioBufIndex, len1, player->m_volume);
        len -= len1;
        player->m_audioBufIndex += len1;
        stream += len1;
    }
    player->m_audioClock.setClock(audioPts);
    uint32_t _pts = (uint32_t)audioPts;
    if (player->m_lastAudioPts != _pts)
    {
        emit player->AVPtsChanged(_pts);
        player->m_lastAudioPts = _pts;
    }
}
int AVPlayer::initAudio()
{
    if (SDL_Init(SDL_INIT_AUDIO) != 0)
    {
        printf("SDL_Init failed\n");
        return 0;
    }

    m_exit = 0;

    m_audioBufSize = 0;
    m_audioBufIndex = 0;
    m_lastAudioPts = -1;
    m_audioCodecPar = m_pDecoder->audioCodecPar();
    int outSample = m_audioCodecPar->sample_rate * m_speed;
    SDL_AudioSpec wanted_spec;

    wanted_spec.channels = m_audioCodecPar->channels;
    wanted_spec.freq = outSample;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.silence = 0;
    wanted_spec.callback = audioCallBack;
    wanted_spec.userdata = this;
    wanted_spec.samples = FFMAX(512, 2 << av_log2(wanted_spec.freq / 30));
    // m_audioCodecPar->frame_size;

    if (SDL_OpenAudio(&wanted_spec, NULL) < 0)
    {
        printf("SDL_OpenAudio failed\n");
        return 0;
    }
    m_targetSamplefmt = AV_SAMPLE_FMT_S16;
    m_targetChannels = m_audioCodecPar->channels;
    m_targetFreq = m_audioCodecPar->sample_rate;
    m_targetChannelLayout = av_get_default_channel_layout(m_audioCodecPar->channels);
    m_targetNbSameples = m_audioCodecPar->frame_size;
    m_audioIndex = m_pDecoder->audioIndex();
    m_pFormatContext = m_pDecoder->m_fmtCtx;

    SDL_PauseAudio(0);
    return 1;
}
void AVPlayer::clearPlayer()
{
    if (getPlayState() != AV_STOPED)
    {
        m_exit = 1;
        if (getPlayState() == AV_PLAYING)
            SDL_PauseAudio(1);
        m_pDecoder->exit();
        SDL_CloseAudio();
        if (m_swrCtx)
            swr_free(&m_swrCtx);
        if (m_swsCtx)
            sws_freeContext(m_swsCtx);
        // delete m_pDecoder;
        // m_pDecoder = nullptr;
        m_swrCtx = nullptr;
        m_swsCtx = nullptr;
    }
}
void AVPlayer::initAVClock()
{
    m_audioClock.setClock(0.00);
    m_vedioClock.setClock(0.00);
    m_clockInitFlag = 1;
}

void AVPlayer::displayImage(AVFrame *frame)
{
    if (frame)
    {
        bool res = VedioFFmpeg::Get()->ToRGB(frame, m_pixels, m_pitch, m_imageWidth, m_imamgeHeight);
        // emit frameChanged(QSharedPointer<YUV420Frame>::create(m_pixels[0], m_imageWidth, m_imamgeHeight));
        if (res == true)
        {
            m_vedioClock.setClock(frame->pts * av_q2d(m_pFormatContext->streams[m_videoIndex]->time_base));
        }
        else
        {
            // printf("decode frame is null\n");
            return;
        }
    }
    else
    {
        printf("decode frame is null\n");
    }
}

void AVPlayer::videoCallBack(std::shared_ptr<void> par)
{
    // 初始化时钟
    av_usleep(100);
    double time = 0.00;
    double duration = 0.00;
    double delay = 0.00;
    if (m_clockInitFlag == -1)
    {
        initAVClock();
    }
    do
    {
        if (m_exit)
            break;
        if (m_pause)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        // 队列中未显示帧一帧以上执行逻辑判断
        if (m_pDecoder->getRemainingVFrame())
        {
            MyFrame *lastframe = m_pDecoder->peekLastVframe();

            MyFrame *frame = m_pDecoder->peekVframe();

            // 检查帧序列是否为空，如果是，则跳过
            if (frame->serial != m_pDecoder->vidPktSerial())
            {
                m_pDecoder->setNextVFrame();
                continue;
            }
            if (frame->serial != lastframe->serial)
            {
                m_frameTimer = av_gettime_relative() / 1000000.0;
            }
            // 计算持续时间
            duration = vpDuration(frame, lastframe);
            delay = computeTargetPlay(duration);
            time = av_gettime_relative() / 1000000.0; // time为实际时钟
            // 实际时钟小于理论时钟,显示时长未到，等待实际时钟追上
            if (time < m_frameTimer + delay)
            {
                QThread::msleep((uint32_t)(FFMIN(AV_SYNC_REJUDGESHOLD, m_frameTimer + delay - time) * 1000));
                continue;
            }
            // 更新理论时钟
            m_frameTimer += delay;
            // 检查理论时钟是否已经超时，如果是，则理论主时钟
            if (time - m_frameTimer > AV_SYNC_THRESHOLD_MAX)
            {
                m_frameTimer = time;
            }
            // 队列中未显示帧一帧以上执行逻辑判断
            if (m_pDecoder->getRemainingVFrame() > 1)
            {
                MyFrame *nextFrame = m_pDecoder->peekNextVFrame();
                duration = nextFrame->pts - frame->pts;
                // 若主时钟超前到大于当前的帧理论显示应持续的时间，则丢弃跟进
                if (time > m_frameTimer + duration)
                {
                    m_pDecoder->setNextVFrame();
                    printf("abandon the current frame\n");
                    continue;
                }
            }
            // 显示图像
            displayImage(&frame->frame);
            // 读索引后移
            m_pDecoder->setNextVFrame();
        }
        else
        {
            QThread::msleep(10);
        }
    } while (1);
    printf("videoCallBack exit\n");
}

// void AVPlayer::audioCallBack(uint8_t *m_audiobuffer, std::shared_ptr<void> par)
// {
//     // 加锁
//     m_mutex.lock();

//     m_audioIndex = m_pDecoder->audioIndex();
//     int errtimes = 10;
//     double currenttime = 0.00;
//     double nexttime = 0.00;
//     QByteArray audioBuffer;
//     long time = 1024 * 1000000 / 44100;
//     while (1)
//     {

//         int ret = m_pDecoder->getAFrame(m_audioFrame);
//         if (ret != 1)
//         {
//             if (errtimes == 0)
//             {
//                 printf("Failed to get audio frame.\n");
//                 break;
//             }
//             errtimes--;
//             printf("Retrying...\n");
//             continue;
//         }
//         // // Calculate the next timestamp based on the audio frame's presentation time
//         double currenttime = m_audioFrame->pts * av_q2d(m_pFormatContext->streams[m_audioIndex]->time_base);
//         AVPlayer::GetPlay()->m_audioClock.setClock(currenttime);
//         // // Convert the frame to PCM
//         av_usleep(time);
//         int len = Audio::AudioFFmpeg::GetAudio()->ToPCM(&m_audiobuffer, m_audioFrame, &m_audioBufSize);

//         // // Accumulate audio data in the buffer
//         audioBuffer.append(reinterpret_cast<const char *>(m_audiobuffer), len);
//         // // Check if the accumulated data is enough to write
//         memset(m_audiobuffer, 0, m_audioBufSize);

//         if (output->bytesFree() >= output->periodSize() && audioBuffer.size() > output->periodSize())
//         {

//             int writeSize = min(output->bytesFree(), output->periodSize());

//             io->write(audioBuffer, writeSize);

//             audioBuffer.remove(0, writeSize);
//         }
//         // Clear the audio buffer
//     }

//     // 释放锁
//     m_mutex.unlock();
// }
void AVPlayer::setVolume(int Volume)
{
    m_volume = (Volume * SDL_MIX_MAXVOLUME / 100) % (SDL_MIX_MAXVOLUME + 1);
}
void AVPlayer::setSpeed(float speed)
{
    m_speed = speed;
}
AVPlayer *AVPlayer::GetPlay()
{
    static AVPlayer *play = new AVPlayer();
    return play;
}
double AVPlayer::vpDuration(MyFrame *curFrame, MyFrame *lastFrame)
{
    if (curFrame->serial == lastFrame->serial)
    {
        // 计算当前帧和上一帧的时间差
        double duration = curFrame->pts - lastFrame->pts;
        // 如果时间差是NAN或大于等于AV_NOSYNC_THRESHOLD,则返回上一帧的持续时间
        if (isnan(duration) || duration > AV_NOSYNC_THRESHOLD)
        {
            return lastFrame->duration;
        }
        else
        {
            return duration;
        }
    }
    return 0.0;
}

double AVPlayer::computeTargetPlay(double delay)
{
    double diff = m_vedioClock.getClock() - m_audioClock.getClock();
    // 计算同步阈值
    double sync = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
    // 不同步时间超过阈值直接放弃同步
    if (!isnan(diff) && abs(diff) < AV_NOSYNC_THRESHOLD)
    {
        // 视频比音频慢，加快,diff为负值
        if (diff <= -sync)
        {
            // 不同步时间超过阈值，但当前时间戳与视频当前显示帧时间戳差值大于阈值，则将delay设置为当前时间戳与视频当前显示帧时间戳差值加上delay
            delay = FFMAX(0, diff + delay);
        }
        // 视频比音频快，减慢
        else if (diff >= sync && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
        {
            // 不同步时间超过阈值，但当前时间戳与视频当前显示帧时间戳差值小于阈值，则将delay设置为当前时间戳与视频当前显示帧时间戳差值加上delay
            delay = diff + delay;
        }
        // 视频比音频快，减慢
        else if (diff >= sync)
        {
            // 不同步时间小于阈值，且当前时间戳与视频当前显示帧时间戳差值小于阈值，则将delay设置为当前时间戳与视频当前显示帧时间戳差值加上delay的2倍
            delay = 2 * delay;
        }
    }
    return delay;
}
