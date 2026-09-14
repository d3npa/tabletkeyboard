#pragma once

#include "ui/themepainter.h"

#include <QPoint>
#include <QSet>
#include <QSize>
#include <QString>
#include <QVector>
#include <QWidget>

#include <memory>

class QTimer;
class QVBoxLayout;

namespace osk {

struct Block;
struct ThemeSpec;
class KeyStateMachine;
class LayoutLibrary;
class ThemeLibrary;
class TitleBar;

// The focusless, frameless, always-on-top floating keyboard. It never injects
// anything itself: taps go to the state machine, which routes scripts to the
// input backend.
class KeyboardWindow : public QWidget
{
    Q_OBJECT
public:
    KeyboardWindow(KeyStateMachine *machine, const ThemeLibrary *themes, QWidget *parent = nullptr);

    void setScale(double scale);
    void setThemeId(const QString &id);
    QString themeId() const { return themeId_; }

    // Ids of the blocks and rows that are not shown ("frow", "numpad").
    void setHiddenBlocks(const QSet<QString> &ids);

    // Title-bar label only: the theme itself is chosen by the app.
    void setDarkMode(bool on);

    // Absolute key size in px; 0 = the theme's own key_unit.
    void setKeyUnit(int px);

    // Rebuilds the key grid (mode/layer/layout/theme changes). Deferred, so it
    // is safe to call from a key's own event handler.
    void rebuild();

    // Applies a pending rebuild immediately (before showing/sizing the window).
    void ensureBuilt();

    void setInputStatus(bool available, const QString &reason);

    QString screenName() const;
    void restorePosition(const QPoint &pos);
    QPoint defaultPosition() const;

signals:
    void hideRequested();
    void positionChanged(const QPoint &pos, const QString &screenName);
    void darkModeToggleRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private:
    void doRebuild();
    QString modeTitle() const;
    QWidget *buildContent(const QVector<const Block *> &blocks, int gap, double uiScale);
    void beginDrag(const QPoint &globalPos);
    void dragTo(const QPoint &globalPos);
    void endDrag();
    void savePositionSoon();

    KeyStateMachine *machine_;
    const ThemeLibrary *themes_;
    std::unique_ptr<ThemePainter> painter_;
    TitleBar *titleBar_ = nullptr;
    QVBoxLayout *rootLayout_ = nullptr;
    QWidget *content_ = nullptr;
    QTimer *positionTimer_ = nullptr;

    QString themeId_;
    QSet<QString> hiddenBlocks_;
    double scale_ = 1.0;
    double unit_ = 44.0;
    int keyUnit_ = 0;
    bool darkMode_ = true;
    bool inputAvailable_ = true;
    QString inputWarning_;
    bool rebuildPending_ = false;
    bool dragging_ = false;
    QPoint dragOffset_;
};

} // namespace osk
