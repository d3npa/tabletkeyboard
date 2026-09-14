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
    KeyButton(const KeyDef &key, KeyStateMachine *machine, const ThemePainter *painter, double scale,
              int unitPx, int rowHeight, QWidget *parent = nullptr);

    QSize sizeHint() const override;
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
    int unitPx_;
    int rowHeight_;
    bool hovered_ = false;
    bool pressed_ = false;
};

} // namespace osk
