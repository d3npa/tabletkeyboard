#include "app/app.h"

#include "app/autostart.h"
#include "app/icon.h"
#include "app/settingsdialog.h"
#include "app/tray.h"
#include "core/layout.h"
#include "core/theme.h"
#include "platform/input_backend.h"
#include "platform/window_adapter.h"
#include "platform/x11/input_xtest.h"
#include "platform/x11/symresolver_x11.h"
#include "platform/x11/window_x11.h"
#include "ui/keyboardwindow.h"

#include <QApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QSet>
#include <QTimer>

namespace osk {

App::App(QObject *parent) : QObject(parent) {}

App::~App()
{
    if (input_)
        input_->shutdown();
}

App::InitResult App::init(QString *error)
{
    settings_.load();

    single_ = new SingleInstance(this);
    const SingleInstance::ClaimResult claim = single_->claim(error);
    if (claim == SingleInstance::Secondary)
        return Secondary;
    if (claim == SingleInstance::Failed)
        return Failed;
    connect(single_, &SingleInstance::commandReceived, this, &App::handleArgs);

    if (!xconn_.open(error))
        return Failed;

    resolver_.reset(new XlibKeysymResolver);
    layouts_ = new LayoutLibrary(resolver_.get());
    layouts_->scan();
    themes_ = new ThemeLibrary();
    themes_->scan();
    if (!layouts_->byId(settings_.layoutId))
        settings_.layoutId = layouts_->sets().isEmpty() ? QString() : layouts_->sets().first().id;
    if (!themes_->byId(settings_.lightTheme))
        settings_.lightTheme = themes_->themes().isEmpty() ? QString() : themes_->themes().first().id;
    if (!themes_->byId(settings_.darkTheme))
        settings_.darkTheme = themes_->themes().isEmpty() ? QString() : themes_->themes().first().id;

    const QString xtestReason = xconn_.xtestAvailable()
            ? QString()
            : QStringLiteral("The XTEST extension is not available on this display; "
                             "the keyboard cannot inject keys.");
    input_.reset(new x11::XTestInputBackend(xconn_.display(), xconn_.xtestAvailable(), xtestReason));
    windowAdapter_.reset(new x11::X11WindowAdapter(xconn_.display(), settings_.onAllDesktops));

    machine_ = new KeyStateMachine(resolver_.get(), layouts_, this);
    machine_->setLayoutId(settings_.layoutId);
    machine_->setMode(settings_.modeId);
    machine_->setStickyTimeoutMs(settings_.stickyTimeoutMs);
    machine_->setZenkakuOnLangSwitch(settings_.zenkakuOnLangSwitch);

    window_ = new KeyboardWindow(machine_, themes_);
    window_->setScale(settings_.scale);
    window_->setThemeId(settings_.themeId());
    window_->setDarkMode(settings_.darkMode);
    window_->setKeyUnit(settings_.keyUnit);
    window_->setShowKana(settings_.showKana);
    window_->setShowIndicators(settings_.showIndicators);
    applyBlocks();
    window_->setInputStatus(input_->available(), input_->unavailableReason());
    window_->rebuild();

    connect(machine_, &KeyStateMachine::output, this, &App::onOutput);
    connect(window_, &KeyboardWindow::hideRequested, this, &App::hideKeyboard);
    connect(window_, &KeyboardWindow::darkModeToggleRequested, this, &App::toggleDarkMode);
    connect(window_, &KeyboardWindow::settingsRequested, this, &App::showSettings);
    connect(window_, &KeyboardWindow::positionChanged, this, &App::onWindowPositionChanged);
    connect(&xconn_, &X11Connection::capsLockChanged, this, [this](bool on) {
        machine_->setCapsOn(on);
        pushLockStates();
    });
    connect(&xconn_, &X11Connection::numLockChanged, this, [this](bool on) {
        machine_->setNumOn(on);
        pushLockStates();
    });
    connect(&xconn_, &X11Connection::scrollLockChanged, this, [this](bool on) {
        machine_->setScrollOn(on);
        pushLockStates();
    });
    connect(&xconn_, &X11Connection::keymapChanged, this, [this]() {
        if (input_)
            input_->syncKeymap();
    });
    connect(qApp, &QCoreApplication::aboutToQuit, this, &App::saveSettings);

    machine_->setCapsOn(xconn_.capsLockOn());
    machine_->setNumOn(xconn_.numLockOn());
    machine_->setScrollOn(xconn_.scrollLockOn());
    pushLockStates();

    QApplication::setWindowIcon(appIcon());

    tray_ = new Tray(this, this);
    trayFallback_ = !tray_->available();

    saveTimer_ = new QTimer(this);
    saveTimer_->setSingleShot(true);
    saveTimer_->setInterval(600);
    connect(saveTimer_, &QTimer::timeout, this, &App::saveSettings);

    for (const QString &layoutError : layouts_->errors())
        qWarning("tabletkeyboard: layout: %s", qPrintable(layoutError));
    for (const QString &themeError : themes_->errors())
        qWarning("tabletkeyboard: theme: %s", qPrintable(themeError));

    if (!input_->available())
        qWarning("tabletkeyboard: %s", qPrintable(input_->unavailableReason()));

    return Primary;
}

void App::applyInitialSettings()
{
    settings_.startAtLogin = Autostart::isEnabled();

    if (trayFallback_) {
        // Without a tray there would be no way to reach the keyboard.
        qWarning("tabletkeyboard: no system tray available; showing the keyboard at startup");
        showKeyboard();
    }
    saveSettingsSoon();
}

void App::forwardArgs(const QStringList &args) const
{
    if (single_)
        single_->forward(args);
}

void App::handleArgs(const QStringList &args)
{
    bool show = false;
    bool hide = false;
    bool toggle = false;
    bool dark = false;
    bool light = false;
    QString mode;
    QString language;
    QString theme;
    QString blocks;
    double scale = -1;

    for (int i = 0; i < args.size(); ++i) {
        const QString arg = args.at(i);
        const auto valueFor = [&](const char *name, QString *target) {
            const QString prefix = QLatin1String(name) + QLatin1Char('=');
            if (arg == QLatin1String(name) && i + 1 < args.size()) {
                *target = args.at(++i);
                return true;
            }
            if (arg.startsWith(prefix)) {
                *target = arg.mid(prefix.size());
                return true;
            }
            return false;
        };

        if (arg == QLatin1String("--show"))
            show = true;
        else if (arg == QLatin1String("--hide"))
            hide = true;
        else if (arg == QLatin1String("--toggle"))
            toggle = true;
        else if (arg == QLatin1String("--dark"))
            dark = true;
        else if (arg == QLatin1String("--light"))
            light = true;
        else if (valueFor("--mode", &mode)) {
        } else if (valueFor("--lang", &language)) {
        } else if (valueFor("--theme", &theme)) {
        } else if (valueFor("--blocks", &blocks)) {
        } else if (arg.startsWith(QLatin1String("--scale"))) {
            QString text;
            if (valueFor("--scale", &text))
                scale = text.toDouble();
        }
    }

    // Dark/light first: --theme then picks the theme of the active slot.
    if (dark)
        setDarkMode(true);
    if (light)
        setDarkMode(false);
    if (!theme.isEmpty())
        setThemeId(theme);
    if (!mode.isEmpty())
        setModeId(mode);
    if (!language.isEmpty())
        setLayoutId(language);
    if (scale > 0)
        setScale(scale);
    if (!blocks.isEmpty()) {
        const QStringList shown = blocks.split(QLatin1Char(','), Qt::SkipEmptyParts);
        setBlockVisible(QStringLiteral("frow"), shown.contains(QStringLiteral("frow")));
        setBlockVisible(QStringLiteral("numpad"), shown.contains(QStringLiteral("numpad")));
    }

    if (show)
        showKeyboard();
    else if (hide)
        hideKeyboard();
    else if (toggle)
        toggleKeyboard();
}

void App::showKeyboard()
{
    if (!window_)
        return;

    // Make sure the key grid exists before the window is sized and placed.
    window_->ensureBuilt();

    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    const QPoint saved = screen ? settings_.positionFor(screen->name()) : QPoint();
    if (screen && !saved.isNull())
        window_->restorePosition(saved);
    else
        window_->move(window_->defaultPosition());

    // EWMH properties must be in place before the window is mapped: KWin reads
    // them when it starts managing the window and ignores later direct writes.
    const quintptr windowId = quintptr(window_->winId());
    if (windowAdapter_) {
        windowAdapter_->configure(windowId);
        windowAdapter_->setOnAllDesktops(windowId, settings_.onAllDesktops);
    }

    window_->show();
    window_->raise();

    // WMs that normalize the desktop/state while they start managing the window
    // (KWin does) need the request repeated once it is actually managed; the
    // pre-map writes above cover WMs that read the properties at manage time.
    if (windowAdapter_ && settings_.onAllDesktops) {
        QTimer::singleShot(150, this, [this, windowId]() {
            if (windowAdapter_ && window_ && window_->isVisible())
                windowAdapter_->setOnAllDesktops(windowId, settings_.onAllDesktops);
        });
    }
}

void App::hideKeyboard()
{
    if (!window_)
        return;
    if (window_->isVisible())
        settings_.setPosition(window_->screenName(), window_->pos());
    window_->hide();
    saveSettingsSoon();
}

void App::toggleKeyboard()
{
    if (isKeyboardVisible())
        hideKeyboard();
    else
        showKeyboard();
}

bool App::isKeyboardVisible() const
{
    return window_ && window_->isVisible();
}

void App::setThemeId(const QString &id)
{
    if (!themes_->byId(id))
        return;
    settings_.setThemeId(id);
    window_->setThemeId(id);
    window_->rebuild();
    saveSettingsSoon();
}

void App::setDarkMode(bool on)
{
    if (settings_.darkMode == on)
        return;
    settings_.darkMode = on;
    window_->setDarkMode(on);
    window_->setThemeId(settings_.themeId());
    window_->rebuild();
    saveSettingsSoon();
}

void App::toggleDarkMode()
{
    setDarkMode(!settings_.darkMode);
}

void App::setBlockVisible(const QString &id, bool visible)
{
    if (id == QLatin1String("frow"))
        settings_.showFrow = visible;
    else if (id == QLatin1String("numpad"))
        settings_.showNumpad = visible;
    else
        return;
    applyBlocks();
    window_->rebuild();
    saveSettingsSoon();
}

void App::applyBlocks()
{
    QSet<QString> hidden;
    if (!settings_.showFrow) {
        hidden.insert(QStringLiteral("frow"));
        hidden.insert(QStringLiteral("frowgap")); // nav/numpad rows that keep level with the F-row
    }
    if (!settings_.showNumpad)
        hidden.insert(QStringLiteral("numpad"));
    window_->setHiddenBlocks(hidden);
}

void App::pushLockStates()
{
    if (window_ && machine_)
        window_->setLockStates(machine_->numOn(), machine_->capsOn(), machine_->scrollOn());
}

void App::setKeyUnit(int px)
{
    settings_.keyUnit = qBound(0, px, 240);
    window_->setKeyUnit(settings_.keyUnit);
    window_->rebuild();
    saveSettingsSoon();
}

void App::setThemeForSlot(bool darkMode, const QString &id)
{
    if (!themes_->byId(id))
        return;
    if (darkMode)
        settings_.darkTheme = id;
    else
        settings_.lightTheme = id;
    if (settings_.darkMode == darkMode) {
        window_->setThemeId(id);
        window_->rebuild();
    }
    saveSettingsSoon();
}

void App::setShowKana(bool on)
{
    settings_.showKana = on;
    window_->setShowKana(on);
    window_->rebuild();
    saveSettingsSoon();
}

void App::setShowIndicators(bool on)
{
    settings_.showIndicators = on;
    window_->setShowIndicators(on);
    saveSettingsSoon();
}

void App::setStickyTimeoutMs(int ms)
{
    settings_.stickyTimeoutMs = qBound(0, ms, 10000);
    machine_->setStickyTimeoutMs(settings_.stickyTimeoutMs);
    saveSettingsSoon();
}

void App::setZenkakuOnLangSwitch(bool on)
{
    settings_.zenkakuOnLangSwitch = on;
    machine_->setZenkakuOnLangSwitch(on);
    saveSettingsSoon();
}

void App::showSettings()
{
    if (!settingsDialog_)
        settingsDialog_.reset(new SettingsDialog(this));
    settingsDialog_->show();
    settingsDialog_->raise();
    settingsDialog_->activateWindow();
}

void App::setModeId(const QString &id)
{
    machine_->setMode(id);
}

void App::setLayoutId(const QString &id)
{
    machine_->setLayoutId(id);
}

void App::setScale(double scale)
{
    settings_.scale = qBound(0.5, scale, 3.0);
    window_->setScale(settings_.scale);
    window_->rebuild();
    saveSettingsSoon();
}

void App::setAutostart(bool on)
{
    QString error;
    if (!Autostart::setEnabled(on, &error)) {
        qWarning("tabletkeyboard: %s", qPrintable(error));
        return;
    }
    settings_.startAtLogin = on;
    saveSettingsSoon();
}

bool App::autostartEnabled() const
{
    return Autostart::isEnabled();
}

void App::onOutput(const osk::Output &out)
{
    if (!out.script.isEmpty() && input_->available())
        input_->execute(out.script);

    if (!out.setMode.isEmpty())
        settings_.modeId = out.setMode;
    if (!out.setLanguage.isEmpty())
        settings_.layoutId = out.setLanguage;

    if (out.viewChanged)
        window_->rebuild();
    else if (out.stateChanged)
        window_->update();

    if (out.hide)
        hideKeyboard();
    if (out.viewChanged)
        saveSettingsSoon();
}

void App::onWindowPositionChanged(const QPoint &pos, const QString &screen)
{
    settings_.setPosition(screen, pos);
    saveSettingsSoon();
}

void App::saveSettingsSoon()
{
    if (saveTimer_)
        saveTimer_->start();
}

void App::saveSettings()
{
    if (window_ && window_->isVisible())
        settings_.setPosition(window_->screenName(), window_->pos());
    settings_.save();
}

} // namespace osk
