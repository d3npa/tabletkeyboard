#pragma once

#include "core/layout.h"

#include <QSize>
#include <QWidget>

namespace osk {

class KeyStateMachine;
class ThemePainter;

// One key of the on-screen keyboard. Custom-painted so themes are pure data;
// taps are forwarded to the state machine, never to the input backend directly.
class KeyButton : public QWidget
{
    Q_OBJECT
public:
    // `size` is the key's full widget rect, `bodyRect` the lower part of a
    // stepped key (JIS Return) in widget coordinates, or empty for a plain
    // rectangle.
    KeyButton(const KeyDef &key, KeyStateMachine *machine, const ThemePainter *painter, double uiScale,
              const QSize &size, const QRect &bodyRect, QWidget *parent = nullptr);

    const KeyDef &keyDef() const { return key_; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    bool isActive() const;
    bool isLocked() const;
    QString displayLabel() const;
    QString subLabel() const;

    KeyDef key_;
    KeyStateMachine *machine_;
    const ThemePainter *painter_;
    double scale_;
    QRect bodyRect_;
    bool hovered_ = false;
    bool pressed_ = false;
};

} // namespace osk
