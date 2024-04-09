#ifndef VFRAME_H
#define VFRAME_H
#include <memory>
#include <stdlib.h>
#include "FFmpegInclude.h"
// YUV420Frame类，用于存储YUV420帧的缓冲区
class YUV420Frame
{
public:
    // 构造函数，传入缓冲区，像素宽和像素高
    YUV420Frame(uint8_t *buffer, uint32_t pixelW, uint32_t pixelH) : m_buffer(nullptr)
    {
        // 创建缓冲区，传入缓冲区，像素宽和像素高
        create(buffer, pixelW, pixelH);
    }
    ~YUV420Frame()
    {
        if (m_buffer)
            free(m_buffer);
    }
    // 获取缓冲区Y
    inline uint8_t *getBufY() const
    {
        return m_buffer;
    }

    // 获取缓冲区U
    inline uint8_t *getBufU() const
    {
        return m_buffer + m_pixelW * m_pixelH;
    }

    // 获取缓冲区V
    inline uint8_t *getBufV() const
    {
        return m_buffer + m_pixelW * m_pixelH * 5 / 4;
    }
    // 在YUV420P格式中，U分量在Y分量之后，占据了Y分量大小的1/4。而V分量在U分量之后，也占据了Y分量大小的1/4。
    // 因此，这些API将根据YUV420P格式的要求，正确地返回对应的Y、U和V分量的指针。
    
    //  获取像素宽
    inline uint32_t getPixelW() const
    {
        return m_pixelW;
    }
    // 获取像素高
    inline uint32_t getPixelH() const
    {
        return m_pixelH;
    }

private:
    void create(uint8_t *buffer, uint32_t pixelW, uint32_t pixelH)
    {
        m_pixelW = pixelW;
        m_pixelH = pixelH;
        int sizeY = pixelW * pixelH;
        int sizeUV = sizeY >> 2;
        if (!m_buffer)
        {
            m_buffer = (uint8_t *)malloc(sizeY + sizeUV * 2);
        }
        if (m_buffer)
        {
            memcpy(m_buffer, buffer, sizeY);                                    // 复制 Y 分量数据
            memcpy(m_buffer + sizeY, buffer + sizeY, sizeUV);                   // 复制 U 分量数据
            memcpy(m_buffer + sizeY + sizeUV, buffer + sizeY + sizeUV, sizeUV); // 复制 V 分量数据
        }
        else
        {
            // 处理内存分配失败的情况
            // ...
            printf("YUV420Frame: 内存分配失败\n");
        }
    }

    uint32_t m_pixelW;
    uint32_t m_pixelH;
    uint8_t *m_buffer;

public:
    static int num;
};

#endif