#include "ui/keybutton.h"
#include "core/keystate.h"
#include "core/keysyms.h"
#include "ui/themepainter.h"

#include <QMouseEvent>
#include <QPainter>

namespace osk {

KeyButton::KeyButton(const KeyDef &key, KeyStateMachine *machine, const ThemePainter *painter, double scale,
                     int unitPx, int rowHeight, QWidget *parent)
    : QWidget(parent), key_(key), machine_(machine), painter_(painter), scale_(scale), unitPx_(unitPx),
      rowHeight_(rowHeight)
{
    setFixedSize(sizeHint());
}

QSize KeyButton::sizeHint() const
{
    return QSize(qRound(key_.width * unitPx_), rowHeight_);
}

bool KeyButton::isActive() const
{
    if (!key_.mod.isEmpty())
        return machine_->modState(key_.mod) != KeyStateMachine::Off;
    if (key_.indicator == QLatin1String("caps"))
        return machine_->capsOn();
    if (key_.indicator == QLatin1String("num"))
        return machine_->numOn();
    return false;
}

bool KeyButton::isLocked() const
{
    return !key_.mod.isEmpty() && machine_->modState(key_.mod) == KeyStateMachine::Locked;
}

QString KeyButton::displayLabel() const
{
    const QString label = key_.label;
    if (machine_->shiftActive() && label.size() == 1 && label.at(0).isLetter())
        return label.toUpper();
    return label;
}

QString KeyButton::subLabel() const
{
    if (key_.shiftedCode == 0)
        return QString();
    const QString text = displayTextForKeysym(key_.shiftedCode);
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
    visual.hovered = hovered_;
    visual.pressed = pressed_;
    visual.active = isActive();
    visual.locked = isLocked();
    painter_->paintKey(painter, rect(), visual, scale_);
}

void KeyButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
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
