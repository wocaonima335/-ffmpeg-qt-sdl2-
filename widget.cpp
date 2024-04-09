#include "widget.h"
#include "./ui_widget.h"
#include "opengl_widget.h"
#include <QMetaMethod>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include "qss.h"
Q_DECLARE_METATYPE(QSharedPointer<YUV420Frame>)
Widget::Widget(QWidget *parent)
    : QWidget(parent), ui(new Ui::Widget), m_duration(1), m_seekTarget(0), m_ptsSliderPressed(false)
{
    ui->setupUi(this);
    init();
    QStringList urls;
    urls << "http://vfx.mtime.cn/Video/2021/11/16/mp4/211116131456748178.mp4";
    urls << "http://vd3.bdstatic.com/mda-jennyc5ci1ugrxzi/mda-jennyc5ci1ugrxzi.mp4";
    urls << "02test.mp4";
    ui->UrlcomboBox->addItems(urls);
    ui->UrlcomboBox->setCurrentIndex(0);
    QStringList Speeds;
    Speeds << QString("1.0x");
    Speeds << QString("1.5x");
    Speeds << QString("2.0x");
    ui->SpeedcomboBox->addItems(Speeds);
    ui->SpeedcomboBox->setCurrentIndex(0);
}

Widget::~Widget()
{
    delete ui;
}
//"http://vfx.mtime.cn/Video/2021/11/16/mp4/211116131456748178.mp4"
void Widget::init()
{
    ui->PtsSlider->setEnabled(false);
    ui->PrepushButton->setEnabled(false);
    ui->NextpushButton->setEnabled(false);
    ui->RePlaypushButton->setEnabled(false);
    qRegisterMetaType<QSharedPointer<YUV420Frame>>("QSharedPointer<YUV420Frame>");
    // AVDecoder::Decoder::getDecoder()->init();
    // AVDecoder::Decoder::getDecoder()->decode("http://vfx.mtime.cn/Video/2021/11/16/mp4/211116131456748178.mp4");
    m_vedio = VedioFFmpeg::Get();
    // bool ok = connect(m_vedio, &VedioFFmpeg::frameChanged, ui->opengl_widget, &OpenGLWidget::onShowYUV, Qt::QueuedConnection);
    bool ok = connect(
        m_vedio, &VedioFFmpeg::frameChanged, ui->opengl_widget, OpenGLWidget::onShowYUV, Qt::QueuedConnection);
    if (ok == true)
    {
        printf("success\n");
    }
    connect(AVPlayer::GetPlay(), &AVPlayer::playStatedChanged, this, &Widget::updatePlayBtn);
    // if (AVPlayer::GetPlay()->m_pause == 0)
    // {
    //     AVPlayer::GetPlay()->play();
    // }
    connect(AVPlayer::GetPlay(), &AVPlayer::AVDurationChanged, this, &Widget::durationChangedSlot);
    connect(AVPlayer::GetPlay(), &AVPlayer::AVPtsChanged, this, &Widget::ptsChangedSlot);
    connect(AVPlayer::GetPlay(), &AVPlayer::AVTerminate, this, &Widget::terminateSlot, Qt::QueuedConnection);

    connect(ui->VolumeSlider, &QSlider::valueChanged, this, &Widget::setVolume);
    connect(ui->PtsSlider, &AVPtsSlider::sliderPressed, this, &Widget::ptsSliderPressedSlot);
    connect(ui->PtsSlider, &AVPtsSlider::sliderMoved, this, &Widget::ptsSliderMovedSlot);
    connect(ui->PtsSlider, &AVPtsSlider::sliderReleased, this, &Widget::ptsSliderReleaseSlot);
    connect(ui->opengl_widget, &OpenGLWidget::mouseClicked, this, &Widget::on_PlaypushButton_clicked);
    connect(ui->opengl_widget, &OpenGLWidget::mouseDoubleClicked, [&]()
            {
        if(this->isMaximized())
        {
            this->showNormal();
        }
        else
        {
            showMaximized();
        } });
    //  connect(ui->PlaypushButton, &QPushButton::clicked, AVPlayer::GetPlay(), &AVPlayer::play);
}
void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setBrush(QBrush(QColor(46, 46, 54)));
    painter.drawRect(rect());
}
void Widget::updatePlayBtn(AVPlayer::PlayState playstate)
{
    if (playstate == AVPlayer::AV_PLAYING)
    {
        ui->PlaypushButton->setStyleSheet(PaseStyle());
    }
    else if (playstate == AVPlayer::AV_PAUSED)
    {
        ui->PlaypushButton->setStyleSheet(PlayStyle());
    }
    else
    {
        ui->PlaypushButton->setStyleSheet(PlayStyle());
    }
}

void Widget::on_NextpushButton_clicked()
{
    AVPlayer::GetPlay()->seekBy(6);
    printf("forward\n");
    if (AVPlayer::GetPlay()->getPlayState() == AVPlayer::AV_PAUSED)
    {
        AVPlayer::GetPlay()->pause(false);
    }
    else
    {
        emit updatePlayBtn(AVPlayer::AV_PAUSED);
    }
}

void Widget::on_PrepushButton_clicked()
{
    AVPlayer::GetPlay()->seekBy(-6);
    printf("comeback\n");
    if (AVPlayer::GetPlay()->getPlayState() == AVPlayer::AV_PAUSED)
    {
        AVPlayer::GetPlay()->pause(false);
    }
    else
    {
        emit updatePlayBtn(AVPlayer::AV_PAUSED);
    }
}

void Widget::setVolume(int volume)
{
    AVPlayer::GetPlay()->setVolume(volume); // 设置音量
}

void Widget::ptsSliderPressedSlot()
{
    m_ptsSliderPressed = true;
    m_seekTarget = (int)(ui->PtsSlider->ptsPercent() * m_duration);
}
void Widget::ptsSliderMovedSlot()
{
    m_seekTarget = (int)(ui->PtsSlider->cursorXPercent() * m_duration);
    const QString &ptsStr = QString("%1:%2").arg(m_seekTarget / 60, 2, 10, QLatin1Char('0')).arg(m_seekTarget % 60, 2, 10, QLatin1Char('0'));
    if (m_ptsSliderPressed)
        ui->Ptslabel->setText(ptsStr);
    else
        ui->PtsSlider->setToolTip(ptsStr);
}

void Widget::ptsSliderReleaseSlot()
{
    AVPlayer::GetPlay()->seekTo(m_seekTarget);
    m_ptsSliderPressed = false;
}

void Widget::durationChangedSlot(unsigned int duration)
{
    ui->Durlabel->setText(QString("%1:%2").arg(duration / 60, 2, 10, QLatin1Char('0')).arg(duration % 60, 2, 10, QLatin1Char('0')));
    m_duration = duration;
}

void Widget::ptsChangedSlot(unsigned int pts)
{
    if (m_ptsSliderPressed)
        return;
    ui->PtsSlider->setPtspercent((double)pts / m_duration);
    ui->Ptslabel->setText(QString("%1:%2").arg(pts / 60, 2, 10, QLatin1Char('0')).arg(pts % 60, 2, 10, QLatin1Char('0')));
}

void Widget::terminateSlot()
{
    ui->Ptslabel->setText(QString("00:00"));
    ui->Durlabel->setText(QString("00:00"));
    ui->PtsSlider->setEnabled(false);
    ui->PrepushButton->setEnabled(false);
    ui->NextpushButton->setEnabled(false);
    ui->RePlaypushButton->setEnabled(false);
    AVPlayer::GetPlay()->clearPlayer();
}

void Widget::on_RePlaypushButton_clicked()
{
    terminateSlot();
    ui->PtsSlider->setEnabled(true);
    ui->PrepushButton->setEnabled(true);
    ui->NextpushButton->setEnabled(true);
    ui->RePlaypushButton->setEnabled(true);
    AVPlayer::GetPlay()->play(ui->UrlcomboBox->currentText().trimmed());
}

void Widget::on_SpeedcomboBox_currentIndexChanged(const QString &arg1)
{
    if (AVPlayer::GetPlay()->isSetSpeed == false)
    {
        return;
    }
    QString test = const_cast<QString &>(arg1);
    AVPlayer::GetPlay()->setSpeed(test.remove("x").toFloat());

    m_seekTarget = (int)(ui->PtsSlider->ptsPercent() * m_duration);
    on_RePlaypushButton_clicked();
    AVPlayer::GetPlay()->seekTo(m_seekTarget);
}

void Widget::on_PlaypushButton_clicked()
{
    switch (AVPlayer::GetPlay()->getPlayState())
    {
    case AVPlayer::AV_PLAYING:
        AVPlayer::GetPlay()->pause(true);
        break;
    case AVPlayer::AV_PAUSED:
        AVPlayer::GetPlay()->pause(false);
        break;
    case AVPlayer::AV_STOPED:
        ui->PtsSlider->setEnabled(true);
        ui->PrepushButton->setEnabled(true);
        ui->NextpushButton->setEnabled(true);
        ui->PlaypushButton->setEnabled(true);
        ui->RePlaypushButton->setEnabled(true);
        AVPlayer::GetPlay()->play(ui->UrlcomboBox->currentText().trimmed());
        AVPlayer::GetPlay()->isSetSpeed = true;
    default:
        break;
    }
}
// QComboBox::drop-down
// {
// 	ubcontrol-origin: padding;
//     subcontrol-position: top right;
//     width: 20px;
//     border-left-width: 1px;
//     border-left-color: rgba(255,255,255,1);
//     border-left-style: solid; /* 设置下拉按钮的样式 */
// }
// QComboBox {
//   background-color:gray ; /* 设置QComboBox的背景颜色 */
//     border:1px solid rgba(255,255,255,1) ; /* 设置QComboBox的边框样式 */
//     border-radius: 2px; /* 设置QComboBox的边框圆角 */
// }
// QComboBox::down-arrow
// {

// 	image: url(:/image-hover/arrow.png);
// background-repeat:no-repeat;
// background-position:center center;
// icon-size:15px;
//   min-width: 15px;
//     min-height: 15px;

// }