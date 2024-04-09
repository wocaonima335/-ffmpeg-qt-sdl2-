#ifndef AUDIOFFMPEG_H
#define AUDIOFFMPEG_H
#include <iostream>
#include "FFmpegInclude.h"
#include "QMutex"
#include <string>
namespace Audio
{

    class AudioFFmpeg
    {
    public:
        AudioFFmpeg();
        ~AudioFFmpeg();
        static AudioFFmpeg *GetAudio();
        int GetPts();
        bool open(const char *path);
        void close();
        std::string GetError();
        AVPacket *Read();
        AVFrame *Decode(const AVPacket *temp);
        int ToPCM(uint8_t **out, AVFrame *Aaframe, uint32_t *index);
        std::string GetDetail();

    public:
        int audioStreamIndex;
        QMutex Audio_frame_mutex;

        uint64_t out_channel_layout;
        int out_sample_rate;
        int out_sample_fmt;
        int out_channels;
        char errorbuff[1024];
        char audioInfo[1024];
        SwrContext *swr_ctx;
        AVFormatContext *Aavformacontext;
        AVCodecContext *Aacodeccontext;
        AVCodec *Aacodec;
        AVFrame *Aaframe;
        AVPacket Aapacket;
        //  AVStream *Astream;
        uint8_t *out_buffer;
    };
}
#endif