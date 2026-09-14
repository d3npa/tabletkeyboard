#pragma once

#include <QObject>

class QMenu;
class QSystemTrayIcon;

namespace osk {

class App;

// Tray access: show/hide, mode, language, theme, scale, autostart, quit.
// The menu is rebuilt on demand so it always reflects the current state.
class Tray : public QObject
{
    Q_OBJECT
public:
    Tray(App *app, QObject *parent = nullptr);
    ~Tray() override;

    bool available() const { return tray_ != nullptr; }

private:
    void rebuildMenu();

    App *app_;
    QSystemTrayIcon *tray_ = nullptr;
    QMenu *menu_ = nullptr;
};

} // namespace osk
