#include "app/app.h"

#include "app/autostart.h"
#include "app/icon.h"
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
    if (!themes_->byId(settings_.themeId))
        settings_.themeId = themes_->themes().isEmpty() ? QString() : themes_->themes().first().id;

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
    window_->setThemeId(settings_.themeId);
    window_->setInputStatus(input_->available(), input_->unavailableReason());
    window_->rebuild();

    connect(machine_, &KeyStateMachine::output, this, &App::onOutput);
    connect(window_, &KeyboardWindow::hideRequested, this, &App::hideKeyboard);
    connect(window_, &KeyboardWindow::positionChanged, this, &App::onWindowPositionChanged);
    connect(&xconn_, &X11Connection::capsLockChanged, machine_, &KeyStateMachine::setCapsOn);
    connect(&xconn_, &X11Connection::numLockChanged, machine_, &KeyStateMachine::setNumOn);
    connect(&xconn_, &X11Connection::keymapChanged, this, [this]() {
        if (input_)
            input_->syncKeymap();
    });
    connect(qApp, &QCoreApplication::aboutToQuit, this, &App::saveSettings);

    machine_->setCapsOn(xconn_.capsLockOn());
    machine_->setNumOn(xconn_.numLockOn());

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
    QString mode;
    QString language;
    QString theme;
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
        else if (valueFor("--mode", &mode)) {
        } else if (valueFor("--lang", &language)) {
        } else if (valueFor("--theme", &theme)) {
        } else if (arg.startsWith(QLatin1String("--scale"))) {
            QString text;
            if (valueFor("--scale", &text))
                scale = text.toDouble();
        }
    }

    if (!theme.isEmpty())
        setThemeId(theme);
    if (!mode.isEmpty())
        setModeId(mode);
    if (!language.isEmpty())
        setLayoutId(language);
    if (scale > 0)
        setScale(scale);

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

    window_->show();
    window_->raise();
    if (windowAdapter_) {
        const quintptr windowId = quintptr(window_->winId());
        windowAdapter_->configure(windowId);
        windowAdapter_->setOnAllDesktops(windowId, settings_.onAllDesktops);
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
    settings_.themeId = id;
    window_->setThemeId(id);
    window_->rebuild();
    saveSettingsSoon();
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
