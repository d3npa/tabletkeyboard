#include "ui/keyboardwindow.h"
#include "core/keystate.h"
#include "core/layout.h"
#include "core/theme.h"
#include "ui/keybutton.h"
#include "ui/themepainter.h"
#include "ui/titlebar.h"

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

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

void KeyboardWindow::setInputStatus(bool available, const QString &reason)
{
    inputAvailable_ = available;
    inputWarning_ = reason;
    if (titleBar_) {
        titleBar_->setStatus(machine_->layout() ? machine_->layout()->name : QString(),
                             modeTitle(), inputAvailable_, inputWarning_);
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

    const int gap = qMax(0, qRound(theme->gap * scale_));
    const int padding = qRound(theme->padding * scale_);
    unit_ = fitUnit(*theme, theme->keyUnit * scale_, gap);
    const int rowHeight = qRound(unit_);

    titleBar_->applyTheme(*theme, scale_);
    titleBar_->setStatus(machine_->layout() ? machine_->layout()->name : QString(), modeTitle(),
                         inputAvailable_, inputWarning_);

    rootLayout_->setContentsMargins(padding, padding, padding, padding);
    rootLayout_->setSpacing(qMax(0, padding / 2));

    if (content_) {
        delete content_;
        content_ = nullptr;
    }
    content_ = buildContent(rowHeight, gap);
    rootLayout_->addWidget(content_);

    setWindowOpacity(theme->windowOpacity);
    rootLayout_->activate();
    setFixedSize(sizeHint());
}

QWidget *KeyboardWindow::buildContent(int rowHeight, int gap)
{
    auto *content = new QWidget(this);
    auto *layout = new QHBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(gap);

    const Layer *layer = machine_->layer();
    if (layer) {
        for (const Block &block : layer->blocks)
            layout->addWidget(buildBlock(block, unit_, rowHeight, gap));
    }
    return content;
}

QWidget *KeyboardWindow::buildBlock(const Block &block, double unit, int rowHeight, int gap)
{
    auto *widget = new QWidget(this);
    auto *column = new QVBoxLayout(widget);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(gap);

    const double blockUnits = block.widthUnits();
    if (block.topGap > 0)
        column->addSpacing(qRound(block.topGap * rowHeight));

    for (const KeyRow &row : block.rows) {
        auto *rowWidget = new QWidget(widget);
        auto *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(gap);

        double rowUnits = 0;
        for (const KeyDef &key : row)
            rowUnits += key.width;
        const int lead = qRound((blockUnits - rowUnits) * unit / 2.0);
        if (lead > 1)
            rowLayout->addSpacing(lead);

        for (const KeyDef &key : row) {
            if (key.type == KeyDef::Spacer) {
                auto *spacer = new QWidget(rowWidget);
                spacer->setAttribute(Qt::WA_TransparentForMouseEvents, true);
                spacer->setFixedSize(qMax(1, qRound(key.width * unit)), rowHeight);
                rowLayout->addWidget(spacer);
            } else {
                auto *button = new KeyButton(key, machine_, painter_.get(), scale_, qRound(unit), rowHeight,
                                             rowWidget);
                rowLayout->addWidget(button);
            }
        }
        rowLayout->addStretch(1);
        column->addWidget(rowWidget);
    }

    column->addStretch(1);
    return widget;
}

double KeyboardWindow::fitUnit(const ThemeSpec &theme, double baseUnit, int gap) const
{
    const Layer *layer = machine_->layer();
    if (!layer || layer->blocks.isEmpty())
        return baseUnit;

    double totalUnits = 0;
    for (const Block &block : layer->blocks)
        totalUnits += block.widthUnits();
    if (totalUnits <= 0)
        return baseUnit;

    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return baseUnit;

    const double margins = 2.0 * theme.padding * scale_;
    const double spacing = double(gap) * qMax(0, layer->blocks.size() - 1);
    const double available = screen->availableGeometry().width() * 0.98 - margins - spacing;
    if (baseUnit * totalUnits <= available)
        return baseUnit;
    return qMax(10.0, available / totalUnits);
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
