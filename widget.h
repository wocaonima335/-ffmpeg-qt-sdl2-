#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "widget.h"
#include "./ui_widget.h"
#include "FFmpegInclude.h"
#include "av_player.h"
#include "opengl_widget.h"
#include "Vframe.h"
#include <QPainter>
#include <QDebug>
#include <QObject>
QT_BEGIN_NAMESPACE
namespace Ui
{
    class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    void init();

private:
    Ui::Widget *ui;
    unsigned int m_duration;
    bool m_ptsSliderPressed;
    int m_seekTarget;

public:
    VedioFFmpeg *m_vedio;

protected:
    virtual void paintEvent(QPaintEvent *event) override;
private slots:
    void on_PlaypushButton_clicked();
    void updatePlayBtn(AVPlayer::PlayState playstate);
    void on_NextpushButton_clicked();
    void on_PrepushButton_clicked();
    void setVolume(int volume);
    void ptsSliderPressedSlot();
    void ptsSliderMovedSlot();
    void ptsSliderReleaseSlot();
    void durationChangedSlot(unsigned int duration);
    void ptsChangedSlot(unsigned int pts);
    void terminateSlot();
    void on_RePlaypushButton_clicked();
    void on_SpeedcomboBox_currentIndexChanged(const QString &arg1);
};
#endif // WIDGET_H


