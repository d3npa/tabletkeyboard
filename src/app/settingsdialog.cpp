// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#include "app/settingsdialog.h"

#include "app/app.h"
#include "app/limits.h"
#include "core/blocks.h"
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
    setWindowTitle(QStringLiteral("Keyboard settings"));
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
    auto *box = new QGroupBox(QStringLiteral("Look"), this);
    auto *form = new QFormLayout(box);

    auto *darkMode = new QCheckBox(QStringLiteral("Dark mode"), box);
    darkMode->setChecked(app_->settings().darkMode);
    connect(darkMode, &QCheckBox::toggled, app_, &App::setDarkMode);
    form->addRow(darkMode);

    auto *lightTheme = makeThemeCombo(false, app_->settings().lightTheme);
    connect(lightTheme, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, lightTheme](int index) {
        app_->setThemeForSlot(false, lightTheme->itemData(index).toString());
    });
    form->addRow(QStringLiteral("Light theme"), lightTheme);

    auto *darkTheme = makeThemeCombo(true, app_->settings().darkTheme);
    connect(darkTheme, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, darkTheme](int index) {
        app_->setThemeForSlot(true, darkTheme->itemData(index).toString());
    });
    form->addRow(QStringLiteral("Dark theme"), darkTheme);

    auto *keyUnit = new QSpinBox(box);
    keyUnit->setRange(limits::minKeyUnitPx, limits::maxKeyUnitPx);
    keyUnit->setSuffix(QStringLiteral(" px"));
    keyUnit->setSpecialValueText(QStringLiteral("theme default"));
    keyUnit->setValue(app_->settings().keyUnit);
    connect(keyUnit, qOverload<int>(&QSpinBox::valueChanged), app_, &App::setKeyUnit);
    form->addRow(QStringLiteral("Key size (0 = theme)"), keyUnit);

    auto *scale = new QDoubleSpinBox(box);
    scale->setRange(limits::minScale, limits::maxScale);
    scale->setSingleStep(0.05);
    scale->setDecimals(2);
    scale->setValue(app_->settings().scale);
    connect(scale, qOverload<double>(&QDoubleSpinBox::valueChanged), app_, &App::setScale);
    form->addRow(QStringLiteral("Scale"), scale);

    return box;
}

QWidget *SettingsDialog::buildKeysSection()
{
    auto *box = new QGroupBox(QStringLiteral("Keys"), this);
    auto *form = new QFormLayout(box);

    auto *frow = new QCheckBox(QStringLiteral("Show F-row"), box);
    frow->setChecked(app_->settings().showFrow);
    connect(frow, &QCheckBox::toggled, this,
            [this](bool on) { app_->setBlockVisible(QString::fromLatin1(blocks::kFrow), on); });
    form->addRow(frow);

    auto *numpad = new QCheckBox(QStringLiteral("Show numpad"), box);
    numpad->setChecked(app_->settings().showNumpad);
    connect(numpad, &QCheckBox::toggled, this,
            [this](bool on) { app_->setBlockVisible(QString::fromLatin1(blocks::kNumpad), on); });
    form->addRow(numpad);

    auto *kana = new QCheckBox(QStringLiteral("Show kana on keys"), box);
    kana->setChecked(app_->settings().showKana);
    connect(kana, &QCheckBox::toggled, app_, &App::setShowKana);
    form->addRow(kana);

    auto *indicators = new QCheckBox(QStringLiteral("Show Num/Caps/Scroll indicators"), box);
    indicators->setChecked(app_->settings().showIndicators);
    connect(indicators, &QCheckBox::toggled, app_, &App::setShowIndicators);
    form->addRow(indicators);

    auto *sticky = new QSpinBox(box);
    sticky->setRange(limits::minStickyTimeoutMs, limits::maxStickyTimeoutMs);
    sticky->setSingleStep(100);
    sticky->setSuffix(QStringLiteral(" ms"));
    sticky->setSpecialValueText(QStringLiteral("never"));
    sticky->setValue(app_->settings().stickyTimeoutMs);
    connect(sticky, qOverload<int>(&QSpinBox::valueChanged), app_, &App::setStickyTimeoutMs);
    form->addRow(QStringLiteral("Sticky modifier timeout"), sticky);

    auto *zenkaku = new QCheckBox(QStringLiteral("Send 半角/全角 on language switch"), box);
    zenkaku->setChecked(app_->settings().zenkakuOnLangSwitch);
    connect(zenkaku, &QCheckBox::toggled, app_, &App::setZenkakuOnLangSwitch);
    form->addRow(zenkaku);

    return box;
}

QWidget *SettingsDialog::buildStartupSection()
{
    auto *box = new QGroupBox(QStringLiteral("Startup"), this);
    auto *form = new QFormLayout(box);

    auto *autostart = new QCheckBox(QStringLiteral("Start at login"), box);
    autostart->setChecked(app_->autostartEnabled());
    connect(autostart, &QCheckBox::toggled, app_, &App::setAutostart);
    form->addRow(autostart);

    return box;
}

} // namespace osk
