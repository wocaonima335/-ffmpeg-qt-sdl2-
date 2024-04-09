#include "widget.h"
#include "AudioFFmpeg.h"
#include "VedioFFmpeg.h"
#include "av_decoder.h"
#include "av_player.h"
#include <QApplication>
#include <thread>
#include "opengl_widget.h"
int main(int argc, char *argv[])
{

    // if (VedioFFmpeg::Get()->Open("02test.mp4"))
    // {
    //     printf("open success %s\n", VedioFFmpeg::Get()->Detail().c_str());
    // }
    // else
    // {
    //     printf("open failed %s\n", VedioFFmpeg::Get()->GetError().c_str()); // 打印c风格字符数组
    //     getchar();
    //     return -1;
    // }
    // AVPacket *pkt;
    // AVFrame *res;
    // char *buf = new char[800 * 600 * 4];
    // for (;;)
    // {
    //     pkt = VedioFFmpeg::Get()->Read();
    //     if (pkt->size == 0)
    //     {
    //         break;
    //     }
    //     printf("pts= %lld\n", pkt->pts);
    //     if (pkt->stream_index != VedioFFmpeg::Get()->VedioStream)
    //     {
    //         av_packet_unref(pkt);
    //         continue;
    //     }
    //     res = VedioFFmpeg::Get()->Decode(pkt); // 可能是视频问题导致解码失败
    //     if (res)
    //     {
    //         printf("success\n");
    //         VedioFFmpeg::Get()->ToRGB(res, buf, 800, 600);
    //     }
    //     else
    //     {
    //         printf("failed\n");
    //     }
    //     av_packet_unref(pkt);
    //     av_frame_unref(res);
    // }
    // VedioFFmpeg::Get()->Close();
    // if (Audio::AudioFFmpeg::GetAudio()->open("02test.mp4"))
    // {
    //     printf("open success %s\n", Audio::AudioFFmpeg::GetAudio()->GetDetail().c_str());
    // }
    // else
    // {
    //     printf("open failed %s\n", Audio::AudioFFmpeg::GetAudio()->GetError().c_str()); // 打印c风格字符数组
    //     getchar();
    //     return -1;
    // }
    // AVPacket *pkt;
    // AVFrame *res;
    // char *buf = new char[10000];
    // memset(buf, 0, 10000); // 将 buf 中的所有字节都设置为 0
    // for (;;)
    // {
    //     pkt = Audio::AudioFFmpeg::GetAudio()->Read();
    //     if (pkt->size == 0)
    //     {
    //         break;
    //     }
    //     printf("pts= %lld\n", pkt->pts);
    //     if (pkt->stream_index != Audio::AudioFFmpeg::GetAudio()->audioStreamIndex)
    //     {

    //         av_packet_unref(pkt);
    //         continue;
    //     }
    //     res = Audio::AudioFFmpeg::GetAudio()->Decode(pkt);
    //     if (res)
    //     {
    //         printf("[A]\n");
    //         Audio::AudioFFmpeg::GetAudio()->ToPCM(buf);
    //     }
    //     av_packet_unref(pkt);
    //     av_frame_unref(res);
    // }
    // Audio::AudioFFmpeg::GetAudio()->close();
    // AVDecoder::Decoder decoder;

    // AVFrame *test = av_frame_alloc();
    // decoder.getAFrame(test);
    // if (test != nullptr)
    // {
    //     printf("get a AFrame success\n");
    // }
    // else
    // {
    //     printf("get a AFrame faild\n");
    // }
    // AVDecoder::Decoder::MyFrame *myframe = decoder.peekLastVframe();
    // if (myframe != nullptr)
    // {
    //     printf("success!!!\n");
    // }
    // else
    // {
    //     printf("failed!!!\n");
    // }
    // myframe = decoder.peekVframe();
    // if (myframe)
    // {
    //     printf("success!!!\n");
    // }
    // else
    // {
    //     printf("failed!!!\n");
    // }
    // myframe = decoder.peekNextVFrame();
    // if (myframe)
    // {
    //     printf("success!!!\n");
    // }
    // else
    // {
    //     printf("failed!!!\n");
    // }
 
    // decoder.decode("02 (120).mp4");
    QApplication a(argc, argv);
    // decoder.demux();
    // AVDecoder::Decoder::getDecoder()->demux(std::make_shared<int>(1));

    // printf(" video  queue of len:%d\n", AVDecoder::Decoder::getDecoder()->m_vedioPacketQueue.size);
    // printf("packet video queue of len %d\n", AVDecoder::Decoder::getDecoder()->m_vedioPacketQueue.size);
    //   printf(" audio  queue of len:%d\n", AVDecoder::Decoder::getDecoder()->m_audioPacketQueue.size);
    // printf("packet video queue of len %d\n", AVDecoder::Decoder::getDecoder()->m_audioPacketQueue.size);
    // // decoder.audioDecoder();
    // printf(" frame audio queue of len:%d\n", AVDecoder::Decoder::getDecoder()->m_audioFrameQueue.size);
    // // AVDecoder::Decoder::getDecoder()->vedioDecoder();
    // printf("frame vedio queue of len: %d\n", AVDecoder::Decoder::getDecoder()->m_vedioFrameQueue.size);
    Widget w;
    w.show();

    return a.exec();
}
