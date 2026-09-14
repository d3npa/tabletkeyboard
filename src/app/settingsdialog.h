#pragma once

#include <QDialog>

class QComboBox;

namespace osk {

class App;

// Live-applying settings dialog: every control writes through the App setters,
// so the keyboard updates while the dialog is open. Non-modal, and it stays
// above the always-on-top keyboard window.
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(App *app, QWidget *parent = nullptr);

private:
    QWidget *buildLookSection();
    QWidget *buildKeysSection();
    QWidget *buildStartupSection();
    QComboBox *makeThemeCombo(bool dark, const QString &current);

    App *app_;
};

} // namespace osk
