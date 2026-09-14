#include "ui/titlebar.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>

namespace osk {

BarButton::BarButton(const QString &text, QWidget *parent) : QWidget(parent), text_(text)
{
    setCursor(Qt::ArrowCursor);
    setAttribute(Qt::WA_Hover, true);
    updateSize();
}

void BarButton::setText(const QString &text)
{
    if (text_ == text)
        return;
    text_ = text;
    updateSize();
    update();
}

void BarButton::applyColors(const QColor &textColor, const QColor &hoverColor, double scale)
{
    textColor_ = textColor;
    hoverColor_ = hoverColor;
    scale_ = scale;
    updateSize();
    update();
}

void BarButton::updateSize()
{
    QFont font = this->font();
    font.setPixelSize(qMax(8, qRound(13 * scale_)));
    const QFontMetrics metrics(font);
    const int width = metrics.horizontalAdvance(text_) + qRound(18 * scale_);
    const int height = qMax(qRound(20 * scale_), metrics.height() + qRound(6 * scale_));
    setFixedSize(width, height);
}

void BarButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (hovered_ || pressed_) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(pressed_ ? hoverColor_.darker(115) : hoverColor_);
        painter.drawRoundedRect(rect(), 3 * scale_, 3 * scale_);
    }
    QFont font = this->font();
    font.setPixelSize(qMax(8, qRound(13 * scale_)));
    painter.setFont(font);
    painter.setPen(textColor_);
    painter.drawText(rect(), Qt::AlignCenter, text_);
}

void BarButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    pressed_ = true;
    update();
    event->accept();
}

void BarButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const bool wasPressed = pressed_;
    pressed_ = false;
    update();
    if (wasPressed && rect().contains(event->pos()))
        emit clicked();
    event->accept();
}

void BarButton::enterEvent(QEvent *event)
{
    hovered_ = true;
    update();
    QWidget::enterEvent(event);
}

void BarButton::leaveEvent(QEvent *event)
{
    hovered_ = false;
    pressed_ = false;
    update();
    QWidget::leaveEvent(event);
}

LockLeds::LockLeds(QWidget *parent) : QWidget(parent)
{
    setToolTip(tr("Num Lock · Caps Lock · Scroll Lock"));
    on_ = QColor(0x4c, 0xaf, 0x50);
    off_ = QColor(0x8a, 0x8a, 0x8a);
    text_ = QColor(0x1a, 0x1a, 0x1a);
    border_ = QColor(0xc8, 0xc8, 0xc8);
    updateSize();
}

void LockLeds::applyColors(const QColor &on, const QColor &off, const QColor &text, const QColor &border,
                           double scale)
{
    on_ = on;
    off_ = off;
    text_ = text;
    border_ = border;
    scale_ = scale;
    updateSize();
    update();
}

void LockLeds::setStates(bool num, bool caps, bool scroll)
{
    if (num_ == num && caps_ == caps && scroll_ == scroll)
        return;
    num_ = num;
    caps_ = caps;
    scroll_ = scroll;
    update();
}

void LockLeds::updateSize()
{
    const int pill = qMax(8, qRound(11 * scale_));
    const int gap = qMax(2, qRound(4 * scale_));
    setFixedSize(3 * pill + 2 * gap, pill);
}

void LockLeds::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int pill = qMax(8, qRound(11 * scale_));
    const int gap = qMax(2, qRound(4 * scale_));
    QFont font = this->font();
    font.setPixelSize(qMax(6, qRound(pill * 0.7)));
    painter.setFont(font);

    const bool states[3] = { num_, caps_, scroll_ };
    const char letters[3] = { '1', 'A', 'S' };
    for (int i = 0; i < 3; ++i) {
        const QRectF box(i * (pill + gap), 0, pill, pill);
        painter.setPen(QPen(border_, 1));
        painter.setBrush(states[i] ? on_ : off_);
        painter.drawRoundedRect(box.adjusted(0.5, 0.5, -0.5, -0.5), 2 * scale_, 2 * scale_);
        painter.setPen(text_);
        painter.drawText(box, Qt::AlignCenter, QString(QLatin1Char(letters[i])));
    }
}

TitleBar::TitleBar(QWidget *parent) : QWidget(parent)
{
    modeButton_ = new BarButton(QStringLiteral("Full"), this);
    languageButton_ = new BarButton(QStringLiteral("EN"), this);
    settingsButton_ = new BarButton(QStringLiteral("⚙"), this);
    settingsButton_->setToolTip(tr("Settings"));
    darkButton_ = new BarButton(QStringLiteral("Dark"), this);
    hideButton_ = new BarButton(QStringLiteral("⌄"), this);
    leds_ = new LockLeds(this);
    warningLabel_ = new QLabel(this);
    warningLabel_->hide();

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);
    layout->addWidget(warningLabel_);
    layout->addStretch(1);
    layout->addWidget(leds_);
    layout->addWidget(modeButton_);
    layout->addWidget(languageButton_);
    layout->addWidget(settingsButton_);
    layout->addWidget(darkButton_);
    layout->addWidget(hideButton_);

    connect(modeButton_, &BarButton::clicked, this, &TitleBar::toggleModeRequested);
    connect(languageButton_, &BarButton::clicked, this, &TitleBar::toggleLanguageRequested);
    connect(settingsButton_, &BarButton::clicked, this, &TitleBar::settingsRequested);
    connect(darkButton_, &BarButton::clicked, this, &TitleBar::toggleDarkModeRequested);
    connect(hideButton_, &BarButton::clicked, this, &TitleBar::hideRequested);
}

void TitleBar::applyTheme(const ThemeSpec &theme, double scale)
{
    barBg_ = QColor(theme.barBg);
    const QColor textColor(theme.keyText);
    QColor hover = QColor(theme.accent);
    hover.setAlpha(45);
    for (BarButton *button : { modeButton_, languageButton_, settingsButton_, darkButton_, hideButton_ })
        button->applyColors(textColor, hover, scale);
    leds_->applyColors(QColor(theme.ledOn), QColor(theme.ledOff), QColor(theme.keyText),
                       QColor(theme.keyBorder), scale);
    warningLabel_->setStyleSheet(QStringLiteral("color: %1;").arg(QColor(theme.accent).name()));
    setFixedHeight(qRound(theme.barHeight * scale));
    update();
}

void TitleBar::setLockStates(bool num, bool caps, bool scroll)
{
    leds_->setStates(num, caps, scroll);
}

void TitleBar::setShowIndicators(bool on)
{
    leds_->setVisible(on);
}

void TitleBar::setStatus(const QString &layoutName, const QString &modeName, bool darkMode,
                         bool inputAvailable, const QString &inputWarning)
{
    modeButton_->setText(modeName);
    languageButton_->setText(layoutName);
    darkButton_->setText(darkMode ? tr("Dark") : tr("Light"));
    if (inputAvailable) {
        warningLabel_->hide();
    } else {
        warningLabel_->setText(QStringLiteral("⚠"));
        warningLabel_->setToolTip(inputWarning);
        warningLabel_->show();
    }
}

void TitleBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), barBg_);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    dragging_ = true;
    emit dragStarted(event->globalPos());
    event->accept();
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging_)
        emit dragMoved(event->globalPos());
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (dragging_) {
        dragging_ = false;
        emit dragFinished();
    }
    QWidget::mouseReleaseEvent(event);
}

} // namespace osk
