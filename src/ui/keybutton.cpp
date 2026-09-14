#include "ui/keybutton.h"
#include "core/keystate.h"
#include "core/keysyms.h"
#include "ui/themepainter.h"

#include <QMouseEvent>
#include <QPainter>

namespace osk {

KeyButton::KeyButton(const KeyDef &key, KeyStateMachine *machine, const ThemePainter *painter, double uiScale,
                     bool showKana, const QSize &size, const QRect &bodyRect, QWidget *parent)
    : QWidget(parent), key_(key), machine_(machine), painter_(painter), scale_(uiScale), showKana_(showKana),
      bodyRect_(bodyRect)
{
    setFixedSize(size);
}

bool KeyButton::isActive() const
{
    if (!key_.mod.isEmpty())
        return machine_->modState(key_.mod) != KeyStateMachine::Off;
    if (key_.indicator == QLatin1String("caps"))
        return machine_->capsOn();
    if (key_.indicator == QLatin1String("num"))
        return machine_->numOn();
    if (key_.indicator == QLatin1String("scroll"))
        return machine_->scrollOn();
    return false;
}

bool KeyButton::isLocked() const
{
    return !key_.mod.isEmpty() && machine_->modState(key_.mod) == KeyStateMachine::Locked;
}

QString KeyButton::displayLabel() const
{
    if (key_.fnCode != 0 && machine_->modState(QLatin1String("fn")) != KeyStateMachine::Off) {
        if (!key_.fnLabel.isEmpty())
            return key_.fnLabel;
        const QString fnText = displayTextForKeysym(key_.fnCode);
        if (!fnText.isEmpty())
            return fnText;
    }
    QString label = key_.label;
    if (machine_->shiftActive() && label.size() == 1 && label.at(0).isLetter())
        return label.toUpper();
    return label;
}

QString KeyButton::subLabel() const
{
    // An explicit hint always wins, so a layout can blank it (`"shiftLabel": ""`)
    // for a keycap that prints no shifted legend while the key keeps sending the
    // shifted keysym.
    if (key_.shiftLabelSet)
        return key_.shiftLabel;
    if (key_.shiftedCode == 0)
        return QString();
    QString text = displayTextForKeysym(key_.shiftedCode);
    if (text.isEmpty() || text.compare(key_.label, Qt::CaseInsensitive) == 0)
        return QString();
    return text;
}

void KeyButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    KeyVisual visual;
    visual.label = displayLabel();
    visual.sublabel = subLabel();
    visual.kana = showKana_ ? key_.kana : QString();
    visual.hovered = hovered_;
    visual.pressed = pressed_;
    visual.active = isActive();
    visual.locked = isLocked();
    painter_->paintKey(painter, rect(), bodyRect_, visual, scale_);
}

void KeyButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    if (!bodyRect_.isEmpty()) {
        // The notch left of a stepped key's lower part belongs to the window,
        // which drags on any press that no key accepts.
        const QRect topPart(0, 0, width(), bodyRect_.top());
        if (!bodyRect_.contains(event->pos()) && !topPart.contains(event->pos())) {
            event->ignore();
            return;
        }
    }
    pressed_ = true;
    update();
    machine_->press(key_);
    event->accept();
}

void KeyButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    pressed_ = false;
    update();
    machine_->release(key_);
    event->accept();
}

void KeyButton::enterEvent(QEvent *event)
{
    hovered_ = true;
    update();
    QWidget::enterEvent(event);
}

void KeyButton::leaveEvent(QEvent *event)
{
    hovered_ = false;
    update();
    QWidget::leaveEvent(event);
}

} // namespace osk
