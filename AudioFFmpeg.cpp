#include "AudioFFmpeg.h"
#include "av_player.h"
Audio::AudioFFmpeg::AudioFFmpeg()
{
    errorbuff[1024] = '\0';
    audioInfo[1024] = '\0';
    av_register_all();       // 注册FFmpeg的库
    avformat_network_init(); // 初始化网络
}
Audio::AudioFFmpeg::~AudioFFmpeg()
{
}

Audio::AudioFFmpeg *Audio::AudioFFmpeg::GetAudio()
{
    static AudioFFmpeg audioffmpeg;
    return &audioffmpeg;
}

bool Audio::AudioFFmpeg::open(const char *path)
{
    close();
    Audio_frame_mutex.lock();
    int res = avformat_open_input(&Aavformacontext, path, NULL, NULL);
    if (res != 0) // 打开
    {
        Audio_frame_mutex.unlock();
        av_strerror(res, errorbuff, 1024);
        printf("open %s failed:%s\n", path, errorbuff);

        return false;
    }
    int totalMs = Aavformacontext->duration / (AV_TIME_BASE / 1000); // 获取音频总时长
    // 获取音频信息
    int ret = avformat_find_stream_info(Aavformacontext, NULL);
    if (ret < 0)
    {
        Audio_frame_mutex.unlock();
        printf("get the message failed");
        av_strerror(res, errorbuff, 1024);

        return false;
    }
    for (int i = 0; i < Aavformacontext->nb_streams; i++)
    {
        Aacodeccontext = Aavformacontext->streams[i]->codec;
        if (Aacodeccontext->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioStreamIndex = i;
            Aacodeccontext = Aavformacontext->streams[audioStreamIndex]->codec;
            Aacodec = avcodec_find_decoder(Aavformacontext->streams[i]->codecpar->codec_id);
            if (!Aacodec)
            {
                Audio_frame_mutex.unlock();

                printf("audio codec no found\n");
                return false;
            }
            av_opt_set(Aacodeccontext->priv_data, "tune", "zerolatency", 0);
            int err = avcodec_open2(Aacodeccontext, Aacodec, nullptr);
            if (err != 0)
            {
                Audio_frame_mutex.unlock();
                char buf[1024] = {0};
                av_strerror(err, errorbuff, sizeof(errorbuff));
                printf(buf);
                return false;
            }
            break;
        }
        if (audioStreamIndex == -1)
        {
            Audio_frame_mutex.unlock();
            printf("not find audio stream\n");
            return false;
        }
    }
    // 获取音频信息
    out_channels = Aavformacontext->streams[audioStreamIndex]->codecpar->channels;
    out_sample_fmt = Aavformacontext->streams[audioStreamIndex]->codecpar->format;
    out_sample_rate = Aavformacontext->streams[audioStreamIndex]->codecpar->sample_rate;
    out_channel_layout = Aavformacontext->streams[audioStreamIndex]->codecpar->channel_layout;

    std::string buffer = std::to_string(out_sample_rate) + " " + std::to_string(out_channels) + " " + std::to_string(out_sample_fmt) + " " + std::to_string(out_channel_layout);
    strcpy(audioInfo, buffer.c_str());
    printf("open %s sucess:%s\n", path, GetDetail().c_str());
    Audio_frame_mutex.unlock();
    return true;
}

void Audio::AudioFFmpeg::close()
{
    Audio_frame_mutex.lock();
    if (Aavformacontext)
    {
        avformat_close_input(&Aavformacontext);
        Aavformacontext = nullptr;
        Aacodeccontext = nullptr;
        Aacodec = nullptr;
    }
    if (Aaframe != nullptr)
    {
        av_frame_free(&Aaframe);
        Aaframe = nullptr;
    }

    if (swr_ctx != nullptr)
    {
        swr_free(&swr_ctx);
        swr_ctx = nullptr;
    }
    Audio_frame_mutex.unlock();
}

std::string Audio::AudioFFmpeg::GetError()
{
    return std::string(errorbuff);
}

AVPacket *Audio::AudioFFmpeg::Read()
{
    memset(&Aapacket, 0, sizeof(Aapacket));
    Audio_frame_mutex.lock();
    if (!Aavformacontext)
    {
        Audio_frame_mutex.unlock();
        return nullptr;
    }
    int err = av_read_frame(Aavformacontext, &Aapacket);
    if (err != 0) // 读取失败
    {
        av_strerror(err, errorbuff, sizeof(errorbuff));
        printf("read frame failed:%s\n", errorbuff);
        Audio_frame_mutex.unlock();
        return nullptr;
    }
    Audio_frame_mutex.unlock();
    return &Aapacket;
}

AVFrame *Audio::AudioFFmpeg::Decode(const AVPacket *temp)
{
    Audio_frame_mutex.lock();
    if (!Aavformacontext)
    {
        Audio_frame_mutex.unlock();
        return nullptr;
    }
    if (Aaframe == nullptr)
    {
        Aaframe = av_frame_alloc();
    }

    int res = avcodec_send_packet(Aacodeccontext, temp);
    if (res < 0 || res == AVERROR(EAGAIN) || res == AVERROR_EOF)
    {
        Audio_frame_mutex.unlock();
        av_strerror(res, errorbuff, sizeof(errorbuff));
        printf("avcodec_send_packet error: %s\n", errorbuff);
        return nullptr;
    }
    res = avcodec_receive_frame(Aacodeccontext, Aaframe);
    if (res != 0)
    {
        Audio_frame_mutex.unlock();
        av_strerror(res, errorbuff, sizeof(errorbuff));
        printf("receive frame failed:%s\n", errorbuff);
        return nullptr;
    }
    Audio_frame_mutex.unlock();

    return Aaframe;
}

int Audio::AudioFFmpeg::ToPCM(uint8_t **out, AVFrame *Aaframe, uint32_t *Size)
{

    Audio_frame_mutex.lock();
    // if (!Aavformacontext || !Aacodec || !out) // 文件未打开，解码器未打开，无数据
    // {
    //     Audio_frame_mutex.unlock();
    //     return -1;
    // }

    Aacodeccontext = Aavformacontext->streams[audioStreamIndex]->codec; // 音频解码器上下文
    if (Aacodeccontext == nullptr)
    {
        Audio_frame_mutex.unlock();
        printf("Adcodeccontext get faild\n");
        return -1;
    }

    // Allocate the swresample context
    swr_ctx = swr_alloc();
    // Set the options
    swr_alloc_set_opts(swr_ctx, Aaframe->channel_layout, AV_SAMPLE_FMT_S16,
                       44100, Aacodeccontext->channel_layout,
                       Aacodeccontext->sample_fmt, Aacodeccontext->sample_rate, 0, 0);
    // Initialize the swresample context
    swr_init(swr_ctx);
    // Unlock the audio frame mutex
    const uint8_t **in = (const uint8_t **)Aaframe->extended_data;
    // int out_cout = (const uint64_t)Aaframe->nb_samples * Aacodeccontext->sample_rate / Aaframe->sample_rate + 256;
    //  if (out_cout <= 0)
    //  {
    //      printf("out_cout failed!!!\n");
    //  }
    //  int out_cout = 10000;
    int outsize = av_samples_get_buffer_size(nullptr, Aaframe->channels, Aaframe->nb_samples, AV_SAMPLE_FMT_S16, 0);
    if (outsize < 0)
    {
        printf("av_samples_get_buffer_size failed\n");
        return -1;
    }
    int bytes_per_sample = av_get_bytes_per_sample((AVSampleFormat)Aaframe->format);
    av_fast_malloc(out, Size, outsize);

    // uint8_t *outs[2];
    // outs[0] = (uint8_t *)malloc(outsize); // len 为4096
    // outs[1] = (uint8_t *)malloc(outsize);
    if (!out)
    {
        return AVERROR(ENOMEM);
    }
    // 音频重采样过程
    // if (!AVPlayer::GetPlay()->m_audiobuffer)
    // {
    //     printf("av_fast_malloc failed\n");
    //     return -1;
    // }
    int len = swr_convert(swr_ctx, out, Aaframe->nb_samples, in, Aaframe->nb_samples);
    if (len < 0)
    {
        Audio_frame_mutex.lock();
        return -1;
    }
    int out_size = av_samples_get_buffer_size(NULL, Aaframe->channels, len, AV_SAMPLE_FMT_S16, 0);

    // printf("outsize = %d\n", out_size);
    //  音频重采样后数据存储到AVPlayer::GetPlay()->m_audiobuffer中

    // double audioPts = 0.00;
    // // 计算音频帧的pts
    // audioPts = Aaframe->pts * av_q2d(Aacodeccontext->time_base);
    // // 设置播放器的时钟
    // AVPlayer::GetPlay()->m_audioClock.setClock(audioPts);

    // 释放音频帧
    av_frame_unref(Aaframe);
    // 解锁音频帧的锁
    Audio_frame_mutex.unlock();

    return out_size;
}

std::string Audio::AudioFFmpeg::GetDetail()
{
    return std::string(audioInfo);
}
