#ifndef MYQSS_H
#define MYQSS_H |
#include "FFmpegInclude.h"
#include <QString>
inline QString PlayStyle()
{
    return "QPushButton"
           " {"
           "background-image:url(:/image-hover/play.png);"
           "   background-repeat:no-repeat;"
           "  background-position:center center;"
           "   border:none;"
           "icon-size:24px;"
           "min-width:24px;"
           "min-height:24px;"
           "    }"

           "QPushButton:hover{"
           "  background-repeat:no-repeat;"
           "   background-position:center center;"
           "background-image:url(:/image-hover/play-hover.png);"

           " }"
           " QPushButton:pressed{"
           "background-repeat:no-repeat;"
           " background-position:center center;"
           " background-image:url(:/image-hover/play.png);"

           "}";
}
inline QString PaseStyle()
{
    return "QPushButton"
           " {"
           "background-image:url(:/image-hover/pase.png);"
           "   background-repeat:no-repeat;"
           "  background-position:center center;"
           "   border:none;"
           "icon-size:24px;"
           "min-width:24px;"
           "min-height:24px;"
           "    }"

           "QPushButton:hover{"
           "  background-repeat:no-repeat;"
           "   background-position:center center;"
           "background-image:url(:/image-hover/pase-hover.png);"
           "   border:none;"
           "icon-size:24px;"
           "min-width:24px;"
           "min-height:24px;"
           " }"

           " QPushButton:pressed{"
           "background-repeat:no-repeat;"
           " background-position:center center;"
           " background-image:url(:/image-hover/pase.png);"
           "   border:none;"
           "icon-size:24px;"
           "min-width:24px;"
           "min-height:24px;"
           "}";
}
inline QString EntrySlot()
{
    return "QSlider::groove:horizontal#PtsSlider{"
           "height: 6px;"
           "left: 0px;"
           "right: 0px;"
           "border: 0px;"
           "border-radius: 30px;"
           "background: rgba(255, 255, 255, 50);"
           "}"

           "QSlider::handle:horizontal#PtsSlider{"
           "width: 10px;"
           "height: 10px;"
           "margin-top: -3px;"
           "margin-left: 0px;"
           "margin-bottom: -3px;"
           "margin-right: 0px;"
           "border-image: url(:/image-hover/sliderHandle.png);"
           "}"

           "QSlider::sub-page:horizontal#PtsSlider{"
           "background: rgba(255, 255, 255, 1)"
           "}";
}
inline QString ReMoveSlot()
{
    return "QSlider::groove:horizontal#PtsSlider{"
           "height: 2px;"
           "left: 0px;"
           "right: 0px;"
           "border: 0px;"
           "border-radius: 30px;"
           "background: rgba(255, 255, 255, 50);"
           "}"

           "QSlider::handle:horizontal#PtsSlider{"
           "width: 10px;"
           "height: 10px;"
           "margin-top: -3px;"
           "margin-left: 0px;"
           "margin-bottom: -3px;"
           "margin-right: 0px;"
           "border-image: url(:/image-hover/sliderHandle.png);"
           "}"

           "QSlider::sub-page:horizontal#PtsSlider{"
           "background: rgba(255, 255, 255, 1)"
           "}";
}
#endif