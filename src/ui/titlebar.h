#pragma once

#include "core/theme.h"

#include <QColor>
#include <QString>
#include <QWidget>

class QLabel;

namespace osk {

// A flat, theme-painted text button for the title bar.
class BarButton : public QWidget
{
    Q_OBJECT
public:
    explicit BarButton(const QString &text, QWidget *parent = nullptr);

    QString text() const { return text_; }
    void setText(const QString &text);
    void applyColors(const QColor &textColor, const QColor &hoverColor, double scale);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void clicked();

private:
    void updateSize();

    QString text_;
    QColor textColor_ = QColor(Qt::black);
    QColor hoverColor_ = QColor(0, 0, 0, 30);
    double scale_ = 1.0;
    bool hovered_ = false;
    bool pressed_ = false;
};

// Three painted pills showing the X server's Num/Caps/Scroll lock state.
class LockLeds : public QWidget
{
    Q_OBJECT
public:
    explicit LockLeds(QWidget *parent = nullptr);

    void applyColors(const QColor &on, const QColor &off, const QColor &text, const QColor &border, double scale);
    void setStates(bool num, bool caps, bool scroll);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateSize();

    bool num_ = false;
    bool caps_ = false;
    bool scroll_ = false;
    double scale_ = 1.0;
    QColor on_;
    QColor off_;
    QColor text_;
    QColor border_;
};

// Drag surface plus [Full/Simple] [Language] [⚙] [Dark] [Hide], the lock
// indicators, and an XTEST warning.
class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);

    void applyTheme(const ThemeSpec &theme, double scale);
    void setStatus(const QString &layoutName, const QString &modeName, bool darkMode, bool inputAvailable,
                   const QString &inputWarning);
    void setLockStates(bool num, bool caps, bool scroll);
    void setShowIndicators(bool on);

signals:
    void toggleModeRequested();
    void toggleLanguageRequested();
    void toggleDarkModeRequested();
    void settingsRequested();
    void hideRequested();
    void dragStarted(const QPoint &globalPos);
    void dragMoved(const QPoint &globalPos);
    void dragFinished();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    LockLeds *leds_;
    BarButton *modeButton_;
    BarButton *languageButton_;
    BarButton *settingsButton_;
    BarButton *darkButton_;
    BarButton *hideButton_;
    QLabel *warningLabel_;
    QColor barBg_ = QColor(0xf4, 0xf4, 0xf4);
    bool dragging_ = false;
};

} // namespace osk
