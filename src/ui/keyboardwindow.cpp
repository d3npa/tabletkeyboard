#include "ui/keyboardwindow.h"
#include "core/geometry.h"
#include "core/keystate.h"
#include "core/layout.h"
#include "core/theme.h"
#include "ui/keybutton.h"
#include "ui/themepainter.h"
#include "ui/titlebar.h"

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

#include <limits>

namespace osk {

KeyboardWindow::KeyboardWindow(KeyStateMachine *machine, const ThemeLibrary *themes, QWidget *parent)
    : QWidget(parent), machine_(machine), themes_(themes)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setWindowTitle(QStringLiteral("tabletkeyboard"));

    titleBar_ = new TitleBar(this);
    connect(titleBar_, &TitleBar::toggleModeRequested, this, [this]() { machine_->toggleMode(); });
    connect(titleBar_, &TitleBar::toggleLanguageRequested, this, [this]() { machine_->cycleLayout(); });
    connect(titleBar_, &TitleBar::hideRequested, this, &KeyboardWindow::hideRequested);
    connect(titleBar_, &TitleBar::dragStarted, this, &KeyboardWindow::beginDrag);
    connect(titleBar_, &TitleBar::dragMoved, this, &KeyboardWindow::dragTo);
    connect(titleBar_, &TitleBar::dragFinished, this, &KeyboardWindow::endDrag);
    connect(titleBar_, &TitleBar::toggleDarkModeRequested, this, &KeyboardWindow::darkModeToggleRequested);

    rootLayout_ = new QVBoxLayout(this);
    rootLayout_->setContentsMargins(0, 0, 0, 0);
    rootLayout_->setSpacing(0);
    rootLayout_->addWidget(titleBar_);

    positionTimer_ = new QTimer(this);
    positionTimer_->setSingleShot(true);
    positionTimer_->setInterval(400);
    connect(positionTimer_, &QTimer::timeout, this, [this]() { emit positionChanged(pos(), screenName()); });

    if (!themes_->themes().isEmpty())
        themeId_ = themes_->themes().first().id;
}

void KeyboardWindow::setScale(double scale)
{
    scale_ = qBound(0.5, scale, 3.0);
}

void KeyboardWindow::setThemeId(const QString &id)
{
    themeId_ = id;
}

void KeyboardWindow::setHiddenBlocks(const QSet<QString> &ids)
{
    hiddenBlocks_ = ids;
}

void KeyboardWindow::setDarkMode(bool on)
{
    darkMode_ = on;
}

void KeyboardWindow::setKeyUnit(int px)
{
    keyUnit_ = qMax(0, px);
}

void KeyboardWindow::setInputStatus(bool available, const QString &reason)
{
    inputAvailable_ = available;
    inputWarning_ = reason;
    if (titleBar_) {
        titleBar_->setStatus(machine_->layout() ? machine_->layout()->name : QString(),
                             modeTitle(), darkMode_, inputAvailable_, inputWarning_);
    }
}

QString KeyboardWindow::modeTitle() const
{
    const Mode *mode = machine_->mode();
    if (!mode)
        return QString();
    QString title = mode->name;
    if (!title.isEmpty())
        title[0] = title.at(0).toUpper();
    return title;
}

void KeyboardWindow::rebuild()
{
    if (rebuildPending_)
        return;
    rebuildPending_ = true;
    QTimer::singleShot(0, this, [this]() {
        if (!rebuildPending_)
            return; // ensureBuilt() already did the work
        rebuildPending_ = false;
        doRebuild();
    });
}

void KeyboardWindow::ensureBuilt()
{
    if (!rebuildPending_)
        return;
    rebuildPending_ = false;
    doRebuild();
}

void KeyboardWindow::doRebuild()
{
    const ThemeSpec *theme = themes_->byId(themeId_);
    if (!theme && !themes_->themes().isEmpty())
        theme = &themes_->themes().first();
    if (!theme)
        return;

    painter_.reset(new ThemePainter(*theme));

    const double gapRatio = double(theme->gap) / theme->keyUnit;
    const double paddingRatio = double(theme->padding) / theme->keyUnit;
    const double baseUnit = keyUnit_ > 0 ? keyUnit_ : theme->keyUnit;

    QVector<const Block *> blocks;
    if (const Layer *layer = machine_->layer()) {
        for (const Block &block : layer->blocks)
            blocks.append(&block);
    }

    const QScreen *primaryScreen = QGuiApplication::primaryScreen();
    const int availableWidth = primaryScreen
            ? primaryScreen->availableGeometry().width() * 98 / 100
            : std::numeric_limits<int>::max();
    const double fit = fitKeyUnit(blocks, hiddenBlocks_, gapRatio, paddingRatio, availableWidth);
    unit_ = qMax(10.0, qMin(fit, scale_ * baseUnit));

    const double uiScale = qMax(0.1, unit_ / theme->keyUnit);
    const int gap = qMax(0, qRound(theme->gap * uiScale));
    const int padding = qRound(theme->padding * uiScale);

    titleBar_->applyTheme(*theme, uiScale);
    titleBar_->setStatus(machine_->layout() ? machine_->layout()->name : QString(), modeTitle(),
                         darkMode_, inputAvailable_, inputWarning_);

    rootLayout_->setContentsMargins(padding, padding, padding, padding);
    rootLayout_->setSpacing(qMax(0, padding / 2));

    if (content_) {
        delete content_;
        content_ = nullptr;
    }
    content_ = buildContent(blocks, gap, uiScale);
    rootLayout_->addWidget(content_);

    setWindowOpacity(theme->windowOpacity);
    rootLayout_->activate();
    setFixedSize(sizeHint());
}

QWidget *KeyboardWindow::buildContent(const QVector<const Block *> &blocks, int gap, double uiScale)
{
    auto *content = new QWidget(this);

    const LayerGeometry geometry = computeLayerGeometry(blocks, hiddenBlocks_, { unit_, gap });
    content->setFixedSize(geometry.size);

    for (const BlockPlacement &block : geometry.blocks) {
        for (const KeyPlacement &placement : block.keys) {
            if (!placement.key)
                continue;
            QWidget *widget = nullptr;
            if (placement.key->type == KeyDef::Spacer) {
                widget = new QWidget(content);
                widget->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            } else {
                widget = new KeyButton(*placement.key, machine_, painter_.get(), uiScale,
                                       placement.rect.size(),
                                       placement.bodyRect.translated(-placement.rect.topLeft()), content);
            }
            widget->setGeometry(placement.rect);
        }
    }
    return content;
}

void KeyboardWindow::paintEvent(QPaintEvent *)
{
    if (painter_) {
        QPainter painter(this);
        painter_->paintBackground(painter, rect());
    }
}

void KeyboardWindow::beginDrag(const QPoint &globalPos)
{
    dragging_ = false;
    if (QWindow *handle = windowHandle()) {
        if (handle->startSystemMove())
            return; // WM-driven move: snapping etc. for free
    }
    dragging_ = true;
    dragOffset_ = globalPos - frameGeometry().topLeft();
}

void KeyboardWindow::dragTo(const QPoint &globalPos)
{
    if (dragging_)
        move(globalPos - dragOffset_);
}

void KeyboardWindow::endDrag()
{
    if (dragging_) {
        dragging_ = false;
        savePositionSoon();
    }
}

void KeyboardWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    beginDrag(event->globalPos());
    event->accept();
}

void KeyboardWindow::mouseMoveEvent(QMouseEvent *event)
{
    dragTo(event->globalPos());
    QWidget::mouseMoveEvent(event);
}

void KeyboardWindow::mouseReleaseEvent(QMouseEvent *event)
{
    endDrag();
    QWidget::mouseReleaseEvent(event);
}

void KeyboardWindow::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    savePositionSoon();
}

void KeyboardWindow::savePositionSoon()
{
    if (positionTimer_)
        positionTimer_->start();
}

QString KeyboardWindow::screenName() const
{
    if (const QScreen *screen = QGuiApplication::screenAt(frameGeometry().center()))
        return screen->name();
    if (QWindow *handle = windowHandle()) {
        if (QScreen *screen = handle->screen())
            return screen->name();
    }
    if (const QScreen *primary = QGuiApplication::primaryScreen())
        return primary->name();
    return QString();
}

void KeyboardWindow::restorePosition(const QPoint &pos)
{
    QScreen *screen = QGuiApplication::screenAt(pos);
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen) {
        move(pos);
        return;
    }
    const QRect available = screen->availableGeometry();
    QPoint target = pos;
    target.setX(qBound(available.left(), target.x(), qMax(available.left(), available.right() - width() + 1)));
    target.setY(qBound(available.top(), target.y(), qMax(available.top(), available.bottom() - 20)));
    move(target);
}

QPoint KeyboardWindow::defaultPosition() const
{
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return QPoint(0, 0);
    const QRect available = screen->availableGeometry();
    return QPoint(available.center().x() - width() / 2, available.bottom() - height() - 4);
}

} // namespace osk
