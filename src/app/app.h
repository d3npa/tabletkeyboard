#pragma once

#include "app/settings.h"
#include "app/singleinstance.h"
#include "core/keystate.h"
#include "platform/x11/xconnection.h"

#include <QObject>
#include <QString>
#include <QStringList>

#include <memory>

class QTimer;

namespace osk {

class InputBackend;
class KeyboardWindow;
class LayoutLibrary;
class ThemeLibrary;
class Tray;
class WindowAdapter;
class XlibKeysymResolver;

// Wires everything together: X11 connection, input backend, state machine, UI,
// tray, settings and the single-instance command pipe.
class App : public QObject
{
    Q_OBJECT
public:
    enum InitResult { Primary, Secondary, Failed };

    explicit App(QObject *parent = nullptr);
    ~App() override;

    InitResult init(QString *error);
    void applyInitialSettings();
    void forwardArgs(const QStringList &args) const;

    // CLI arguments, or the same arguments forwarded by a second instance.
    void handleArgs(const QStringList &args);

    void showKeyboard();
    void hideKeyboard();
    void toggleKeyboard();
    bool isKeyboardVisible() const;

    void setThemeId(const QString &id);
    void setModeId(const QString &id);
    void setLayoutId(const QString &id);
    void setScale(double scale);
    void setAutostart(bool on);
    void setDarkMode(bool on);
    void toggleDarkMode();
    void setBlockVisible(const QString &id, bool visible); // "frow" | "numpad"

    const LayoutLibrary *layouts() const { return layouts_; }
    const ThemeLibrary *themes() const { return themes_; }
    KeyStateMachine *machine() const { return machine_; }
    const AppSettings &settings() const { return settings_; }
    double scale() const { return settings_.scale; }
    bool autostartEnabled() const;

private slots:
    void onOutput(const osk::Output &out);
    void onWindowPositionChanged(const QPoint &pos, const QString &screen);
    void saveSettings();

private:
    void saveSettingsSoon();
    void applyBlocks(); // settings -> window

    X11Connection xconn_;
    std::unique_ptr<XlibKeysymResolver> resolver_;
    LayoutLibrary *layouts_ = nullptr;
    ThemeLibrary *themes_ = nullptr;
    std::unique_ptr<InputBackend> input_;
    std::unique_ptr<WindowAdapter> windowAdapter_;
    KeyStateMachine *machine_ = nullptr;
    KeyboardWindow *window_ = nullptr;
    Tray *tray_ = nullptr;
    SingleInstance *single_ = nullptr;
    AppSettings settings_;
    QTimer *saveTimer_ = nullptr;
    bool trayFallback_ = false;
};

} // namespace osk
