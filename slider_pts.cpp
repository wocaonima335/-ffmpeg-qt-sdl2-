#include "slider_pts.h"
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>
#include "qss.h"
AVPtsSlider::AVPtsSlider(QWidget *parent) : QSlider(parent), m_isEnter(false), m_percent(0.00)
{
    setMouseTracking(true);
}

AVPtsSlider::~AVPtsSlider()
{
}

void AVPtsSlider::setPtspercent(double percent)
{
    if (percent < 0.0 || percent > 100.0)
    {
        return;
    }
    m_percent = percent;
    int value = (int)(percent * this->width());
    setValue(value);
}

double AVPtsSlider::ptsPercent()
{
    double percent = 0.0;
    percent = (double)this->value() / this->width();
    return percent;
}
bool AVPtsSlider::event(QEvent *event)
{
    if (event->type() == QEvent::Enter)
    {
        setStyleSheet(EntrySlot());
    }
    else if (event->type() == QEvent::Leave)
    {
        setStyleSheet(ReMoveSlot());
    }
    else if (event->type() == QEvent::Resize)
    {
        setMaximum(this->width() - 1);
        setPtspercent(m_percent);
        m_percent = ptsPercent();
    }
    return QSlider::event(event);
}

void AVPtsSlider::mousePressEvent(QMouseEvent *event)
{
    setValue(event->pos().x());
    emit sliderPressed();
    m_isEnter = true;
}

void AVPtsSlider::mouseMoveEvent(QMouseEvent *event)
{
    int posX = event->pos().x();
    if (posX > width())
        posX = width();
    if (posX < 0)
        posX = 0;
    m_cursorXPer = (double)(posX) / width();
    if (m_isEnter)
        setValue(posX);
    emit sliderMoved();
}

void AVPtsSlider::mouseReleaseEvent(QMouseEvent *e)
{
    m_percent = ptsPercent();
    emit sliderReleased();
    m_isEnter = false;
}

void AVPtsSlider::paintEvent(QPaintEvent *e)
{
    return QSlider::paintEvent(e);
}
