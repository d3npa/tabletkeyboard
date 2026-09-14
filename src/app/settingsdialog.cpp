#include "app/settingsdialog.h"

#include "app/app.h"
#include "core/theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QVBoxLayout>

namespace osk {

SettingsDialog::SettingsDialog(App *app, QWidget *parent) : QDialog(parent), app_(app)
{
    setWindowTitle(tr("Keyboard settings"));
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setModal(false);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(buildLookSection());
    layout->addWidget(buildKeysSection());
    layout->addWidget(buildStartupSection());

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
    layout->addWidget(buttons);
}

QComboBox *SettingsDialog::makeThemeCombo(bool dark, const QString &current)
{
    auto *combo = new QComboBox(this);
    for (const ThemeSpec *theme : app_->themes()->byVariant(dark))
        combo->addItem(theme->name, theme->id);
    const int index = combo->findData(current);
    combo->setCurrentIndex(index >= 0 ? index : 0);
    return combo;
}

QWidget *SettingsDialog::buildLookSection()
{
    auto *box = new QGroupBox(tr("Look"), this);
    auto *form = new QFormLayout(box);

    auto *darkMode = new QCheckBox(tr("Dark mode"), box);
    darkMode->setChecked(app_->settings().darkMode);
    connect(darkMode, &QCheckBox::toggled, app_, &App::setDarkMode);
    form->addRow(darkMode);

    auto *lightTheme = makeThemeCombo(false, app_->settings().lightTheme);
    connect(lightTheme, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, lightTheme](int index) {
        app_->setThemeForSlot(false, lightTheme->itemData(index).toString());
    });
    form->addRow(tr("Light theme"), lightTheme);

    auto *darkTheme = makeThemeCombo(true, app_->settings().darkTheme);
    connect(darkTheme, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, darkTheme](int index) {
        app_->setThemeForSlot(true, darkTheme->itemData(index).toString());
    });
    form->addRow(tr("Dark theme"), darkTheme);

    auto *keyUnit = new QSpinBox(box);
    keyUnit->setRange(0, 240);
    keyUnit->setSuffix(QStringLiteral(" px"));
    keyUnit->setSpecialValueText(tr("theme default"));
    keyUnit->setValue(app_->settings().keyUnit);
    connect(keyUnit, qOverload<int>(&QSpinBox::valueChanged), app_, &App::setKeyUnit);
    form->addRow(tr("Key size (0 = theme)"), keyUnit);

    auto *scale = new QDoubleSpinBox(box);
    scale->setRange(0.5, 3.0);
    scale->setSingleStep(0.05);
    scale->setDecimals(2);
    scale->setValue(app_->settings().scale);
    connect(scale, qOverload<double>(&QDoubleSpinBox::valueChanged), app_, &App::setScale);
    form->addRow(tr("Scale"), scale);

    return box;
}

QWidget *SettingsDialog::buildKeysSection()
{
    auto *box = new QGroupBox(tr("Keys"), this);
    auto *form = new QFormLayout(box);

    auto *frow = new QCheckBox(tr("Show F-row"), box);
    frow->setChecked(app_->settings().showFrow);
    connect(frow, &QCheckBox::toggled, this,
            [this](bool on) { app_->setBlockVisible(QStringLiteral("frow"), on); });
    form->addRow(frow);

    auto *numpad = new QCheckBox(tr("Show numpad"), box);
    numpad->setChecked(app_->settings().showNumpad);
    connect(numpad, &QCheckBox::toggled, this,
            [this](bool on) { app_->setBlockVisible(QStringLiteral("numpad"), on); });
    form->addRow(numpad);

    auto *kana = new QCheckBox(tr("Show kana on keys"), box);
    kana->setChecked(app_->settings().showKana);
    connect(kana, &QCheckBox::toggled, app_, &App::setShowKana);
    form->addRow(kana);

    auto *indicators = new QCheckBox(tr("Show Num/Caps/Scroll indicators"), box);
    indicators->setChecked(app_->settings().showIndicators);
    connect(indicators, &QCheckBox::toggled, app_, &App::setShowIndicators);
    form->addRow(indicators);

    auto *sticky = new QSpinBox(box);
    sticky->setRange(0, 10000);
    sticky->setSingleStep(100);
    sticky->setSuffix(QStringLiteral(" ms"));
    sticky->setSpecialValueText(tr("never"));
    sticky->setValue(app_->settings().stickyTimeoutMs);
    connect(sticky, qOverload<int>(&QSpinBox::valueChanged), app_, &App::setStickyTimeoutMs);
    form->addRow(tr("Sticky modifier timeout"), sticky);

    auto *zenkaku = new QCheckBox(tr("Send 半角/全角 on language switch"), box);
    zenkaku->setChecked(app_->settings().zenkakuOnLangSwitch);
    connect(zenkaku, &QCheckBox::toggled, app_, &App::setZenkakuOnLangSwitch);
    form->addRow(zenkaku);

    return box;
}

QWidget *SettingsDialog::buildStartupSection()
{
    auto *box = new QGroupBox(tr("Startup"), this);
    auto *form = new QFormLayout(box);

    auto *autostart = new QCheckBox(tr("Start at login"), box);
    autostart->setChecked(app_->autostartEnabled());
    connect(autostart, &QCheckBox::toggled, app_, &App::setAutostart);
    form->addRow(autostart);

    return box;
}

} // namespace osk
