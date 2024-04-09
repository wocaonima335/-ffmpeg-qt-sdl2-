#ifndef AVCLOCK_H
#define AVCLOCK_H

#include "FFmpegInclude.h"

class AVClock
{
public:
    AVClock() :m_pts(0.00), m_drift(0.00)
    {
    }
    ~AVClock()
    {

    }
    //重置时间戳和时钟漂移
    inline void reset()
    {
        m_pts = 0.00;
        m_drift = 0.00;
    }
  // 设置时间戳
    inline void setClock(double pts) {
        setCloctAt(pts);
    }

    // 获取时间戳
    inline double getClock() {
        return m_drift+av_gettime_relative()/1000000.0;
    }

private:
    // 设置时间戳
    inline void setCloctAt(double pts) {
        m_drift=pts-av_gettime_relative()/1000000.0;
        m_pts=pts;
    }
private:
    double m_pts;
    double m_drift;
};
#endif