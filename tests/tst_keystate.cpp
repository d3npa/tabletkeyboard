#include "core/keystate.h"
#include "core/keysyms.h"
#include "core/layout.h"
#include "platform/x11/symresolver_x11.h"

#include <QtTest>

using namespace osk;

class TestKeyState : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void tapLetter();
    void shiftOneShot();
    void shiftLocksOnSecondTap();
    void longPressLocks();
    void ctrlChord();
    void fnIsNotAnXModifier();
    void fnAloneSendsNothing();
    void fnGatesNumberRow();
    void fnWinsOverShift();
    void layerSwitch();
    void hideAction();
    void repeatWhileHeld();
    void languageSwitch();

private:
    const KeyDef *findKey(const QString &setId, const QString &modeId, const QString &layerId,
                          const QString &symName) const;
    const KeyDef *findMod(const QString &setId, const QString &modeId, const QString &layerId,
                          const QString &mod) const;
    const KeyDef *findAction(const QString &setId, const QString &modeId, const QString &layerId,
                             const QString &action) const;
    quint32 keysym(const char *name) const;

    XlibKeysymResolver resolver_;
    LayoutLibrary library_{ &resolver_ };
    KeyStateMachine machine_{ &resolver_, &library_ };
    QVector<Output> outputs_;
};

void TestKeyState::initTestCase()
{
    library_.scan();
    QVERIFY(library_.byId(QStringLiteral("us")));
    QVERIFY(library_.byId(QStringLiteral("jp106")));

    machine_.setLayoutId(QStringLiteral("us"));
    machine_.setMode(QStringLiteral("full"));
    connect(&machine_, &KeyStateMachine::output, this,
            [this](const Output &out) { outputs_.append(out); });
}

const KeyDef *TestKeyState::findKey(const QString &setId, const QString &modeId, const QString &layerId,
                                    const QString &symName) const
{
    const LayoutSet *set = library_.byId(setId);
    const Mode *mode = set ? set->mode(modeId) : nullptr;
    const Layer *layer = mode ? mode->layer(layerId) : nullptr;
    if (!layer)
        return nullptr;
    quint32 code = 0;
    if (!resolver_.fromName(symName, &code))
        return nullptr;
    for (const Block &block : layer->blocks) {
        for (const KeyRow &row : block.rows) {
            for (const KeyDef &key : row.keys) {
                if (key.type == KeyDef::Key && key.symCode == code)
                    return &key;
            }
        }
    }
    return nullptr;
}

const KeyDef *TestKeyState::findMod(const QString &setId, const QString &modeId, const QString &layerId,
                                    const QString &mod) const
{
    const LayoutSet *set = library_.byId(setId);
    const Mode *mode = set ? set->mode(modeId) : nullptr;
    const Layer *layer = mode ? mode->layer(layerId) : nullptr;
    if (!layer)
        return nullptr;
    for (const Block &block : layer->blocks) {
        for (const KeyRow &row : block.rows) {
            for (const KeyDef &key : row.keys) {
                if (key.type == KeyDef::Mod && key.mod == mod)
                    return &key;
            }
        }
    }
    return nullptr;
}

const KeyDef *TestKeyState::findAction(const QString &setId, const QString &modeId, const QString &layerId,
                                       const QString &action) const
{
    const LayoutSet *set = library_.byId(setId);
    const Mode *mode = set ? set->mode(modeId) : nullptr;
    const Layer *layer = mode ? mode->layer(layerId) : nullptr;
    if (!layer)
        return nullptr;
    for (const Block &block : layer->blocks) {
        for (const KeyRow &row : block.rows) {
            for (const KeyDef &key : row.keys) {
                if (key.type == KeyDef::Action && key.action == action)
                    return &key;
            }
        }
    }
    return nullptr;
}

quint32 TestKeyState::keysym(const char *name) const
{
    quint32 code = 0;
    if (!resolver_.fromName(QString::fromLatin1(name), &code))
        return 0;
    return code;
}

void TestKeyState::tapLetter()
{
    outputs_.clear();
    const KeyDef *key = findKey(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                                QStringLiteral("a"));
    QVERIFY(key);
    machine_.press(*key);
    machine_.release(*key);

    QCOMPARE(outputs_.size(), 1);
    const KeyScript script = outputs_.first().script;
    QCOMPARE(script.size(), 1);
    QCOMPARE(int(script.first().type), int(KeyAction::Tap));
    QCOMPARE(script.first().keysym, keysym("a"));
}

void TestKeyState::shiftOneShot()
{
    const KeyDef *shift = findMod(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                                  QStringLiteral("shift"));
    const KeyDef *keyA = findKey(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                                 QStringLiteral("a"));
    QVERIFY(shift && keyA);

    machine_.press(*shift);
    machine_.release(*shift);
    QCOMPARE(int(machine_.modState(QStringLiteral("shift"))), int(KeyStateMachine::OneShot));

    outputs_.clear();
    machine_.press(*keyA);
    machine_.release(*keyA);

    QCOMPARE(outputs_.size(), 1);
    const KeyScript script = outputs_.first().script;
    QCOMPARE(script.size(), 3);
    QCOMPARE(script.at(0).keysym, keysym("Shift_L"));
    QCOMPARE(int(script.at(0).type), int(KeyAction::Down));
    QCOMPARE(script.at(1).keysym, keysym("A"));
    QCOMPARE(int(script.at(1).type), int(KeyAction::Tap));
    QCOMPARE(script.at(2).keysym, keysym("Shift_L"));
    QCOMPARE(int(script.at(2).type), int(KeyAction::Up));
    QCOMPARE(int(machine_.modState(QStringLiteral("shift"))), int(KeyStateMachine::Off));

    // The next tap is unshifted again.
    outputs_.clear();
    machine_.press(*keyA);
    machine_.release(*keyA);
    QCOMPARE(outputs_.first().script.size(), 1);
    QCOMPARE(outputs_.first().script.first().keysym, keysym("a"));
}

void TestKeyState::shiftLocksOnSecondTap()
{
    const KeyDef *shift = findMod(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                                  QStringLiteral("shift"));
    const KeyDef *keyA = findKey(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                                 QStringLiteral("a"));
    QVERIFY(shift && keyA);

    machine_.press(*shift);
    machine_.release(*shift);
    machine_.press(*shift);
    machine_.release(*shift);
    QCOMPARE(int(machine_.modState(QStringLiteral("shift"))), int(KeyStateMachine::Locked));

    outputs_.clear();
    machine_.press(*keyA);
    machine_.release(*keyA);
    QCOMPARE(outputs_.first().script.size(), 3);
    QCOMPARE(int(machine_.modState(QStringLiteral("shift"))), int(KeyStateMachine::Locked));

    machine_.press(*shift);
    machine_.release(*shift);
    QCOMPARE(int(machine_.modState(QStringLiteral("shift"))), int(KeyStateMachine::Off));
}

void TestKeyState::longPressLocks()
{
    machine_.setLongPressMs(30);
    const KeyDef *shift = findMod(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                                  QStringLiteral("shift"));
    QVERIFY(shift);

    machine_.press(*shift);
    QTest::qWait(90);
    QCOMPARE(int(machine_.modState(QStringLiteral("shift"))), int(KeyStateMachine::Locked));
    machine_.release(*shift); // the release of a long press only clears the flag
    QCOMPARE(int(machine_.modState(QStringLiteral("shift"))), int(KeyStateMachine::Locked));

    machine_.press(*shift);
    machine_.release(*shift);
    QCOMPARE(int(machine_.modState(QStringLiteral("shift"))), int(KeyStateMachine::Off));
    machine_.setLongPressMs(450);
}

void TestKeyState::ctrlChord()
{
    const KeyDef *ctrl = findMod(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                                 QStringLiteral("ctrl"));
    const KeyDef *keyC = findKey(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                                 QStringLiteral("c"));
    QVERIFY(ctrl && keyC);

    machine_.press(*ctrl);
    machine_.release(*ctrl);

    outputs_.clear();
    machine_.press(*keyC);
    machine_.release(*keyC);

    const KeyScript script = outputs_.first().script;
    QCOMPARE(script.size(), 3);
    QCOMPARE(script.at(0).keysym, keysym("Control_L"));
    QCOMPARE(script.at(1).keysym, keysym("c"));
    QCOMPARE(script.at(2).keysym, keysym("Control_L"));
    QCOMPARE(int(machine_.modState(QStringLiteral("ctrl"))), int(KeyStateMachine::Off));
}

void TestKeyState::fnIsNotAnXModifier()
{
    QVERIFY(isModifierId(QStringLiteral("fn")));
    QVERIFY(modifierKeysymName(QStringLiteral("fn")).isEmpty());
    QVERIFY(modifierIds().contains(QStringLiteral("ctrl")));
    QVERIFY(!modifierIds().contains(QStringLiteral("fn")));
}

void TestKeyState::fnAloneSendsNothing()
{
    const KeyDef *fn = findMod(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                               QStringLiteral("fn"));
    QVERIFY(fn);
    QCOMPARE(int(machine_.modState(QStringLiteral("fn"))), int(KeyStateMachine::Off));

    outputs_.clear();
    machine_.press(*fn);
    machine_.release(*fn);
    QCOMPARE(outputs_.size(), 1);
    QVERIFY(outputs_.first().script.isEmpty());
    QCOMPARE(int(machine_.modState(QStringLiteral("fn"))), int(KeyStateMachine::OneShot));

    machine_.press(*fn); // second tap locks, third unlocks
    machine_.release(*fn);
    machine_.press(*fn);
    machine_.release(*fn);
    QCOMPARE(int(machine_.modState(QStringLiteral("fn"))), int(KeyStateMachine::Off));
}

void TestKeyState::fnGatesNumberRow()
{
    const KeyDef *fn = findMod(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                               QStringLiteral("fn"));
    const KeyDef *one = findKey(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                                QStringLiteral("1"));
    QVERIFY(fn && one);

    machine_.press(*fn);
    machine_.release(*fn);
    outputs_.clear();
    machine_.press(*one);
    machine_.release(*one);

    QCOMPARE(outputs_.size(), 1);
    const KeyScript script = outputs_.first().script;
    QCOMPARE(script.size(), 1); // Fn itself injects nothing
    QCOMPARE(int(script.first().type), int(KeyAction::Tap));
    QCOMPARE(script.first().keysym, keysym("F1"));
    QCOMPARE(int(machine_.modState(QStringLiteral("fn"))), int(KeyStateMachine::Off));

    outputs_.clear();
    machine_.press(*one);
    machine_.release(*one);
    QCOMPARE(outputs_.first().script.first().keysym, keysym("1"));
}

void TestKeyState::fnWinsOverShift()
{
    const KeyDef *shift = findMod(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                                  QStringLiteral("shift"));
    const KeyDef *fn = findMod(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                               QStringLiteral("fn"));
    const KeyDef *one = findKey(QStringLiteral("us"), QStringLiteral("full"), QStringLiteral("main"),
                                QStringLiteral("1"));
    QVERIFY(shift && fn && one);

    machine_.press(*fn);
    machine_.release(*fn);
    machine_.press(*shift);
    machine_.release(*shift);

    outputs_.clear();
    machine_.press(*one);
    machine_.release(*one);

    QCOMPARE(outputs_.size(), 1);
    const KeyScript script = outputs_.first().script;
    QCOMPARE(script.size(), 3);
    QCOMPARE(script.at(0).keysym, keysym("Shift_L"));
    QCOMPARE(int(script.at(0).type), int(KeyAction::Down));
    QCOMPARE(script.at(1).keysym, keysym("F1"));
    QCOMPARE(int(script.at(1).type), int(KeyAction::Tap));
    QCOMPARE(script.at(2).keysym, keysym("Shift_L"));
    QCOMPARE(int(script.at(2).type), int(KeyAction::Up));
    QCOMPARE(int(machine_.modState(QStringLiteral("fn"))), int(KeyStateMachine::Off));
    QCOMPARE(int(machine_.modState(QStringLiteral("shift"))), int(KeyStateMachine::Off));
}

void TestKeyState::layerSwitch()
{
    machine_.setMode(QStringLiteral("simple"));
    QCOMPARE(machine_.layerId(), QStringLiteral("main"));

    const KeyDef *symbols = findAction(QStringLiteral("us"), QStringLiteral("simple"),
                                       QStringLiteral("main"), QStringLiteral("layer"));
    QVERIFY(symbols);
    QCOMPARE(symbols->layer, QStringLiteral("symbols"));

    outputs_.clear();
    machine_.press(*symbols);
    machine_.release(*symbols);
    QCOMPARE(outputs_.first().setLayer, QStringLiteral("symbols"));
    QVERIFY(outputs_.first().viewChanged);
    QCOMPARE(machine_.layerId(), QStringLiteral("symbols"));

    const KeyDef *back = findAction(QStringLiteral("us"), QStringLiteral("simple"),
                                    QStringLiteral("symbols"), QStringLiteral("layer"));
    QVERIFY(back);
    machine_.press(*back);
    machine_.release(*back);
    QCOMPARE(machine_.layerId(), QStringLiteral("main"));

    machine_.setMode(QStringLiteral("full"));
}

void TestKeyState::hideAction()
{
    const KeyDef *hide = findAction(QStringLiteral("us"), QStringLiteral("full"),
                                    QStringLiteral("main"), QStringLiteral("hide"));
    if (!hide) {
        machine_.setMode(QStringLiteral("simple"));
        hide = findAction(QStringLiteral("us"), QStringLiteral("simple"), QStringLiteral("main"),
                          QStringLiteral("hide"));
        machine_.setMode(QStringLiteral("full"));
    }
    QVERIFY(hide);

    outputs_.clear();
    machine_.press(*hide);
    machine_.release(*hide);
    QCOMPARE(outputs_.size(), 1);
    QVERIFY(outputs_.first().hide);
}

void TestKeyState::repeatWhileHeld()
{
    machine_.setRepeatTiming(20, 10);
    const KeyDef *backspace = findKey(QStringLiteral("us"), QStringLiteral("full"),
                                      QStringLiteral("main"), QStringLiteral("BackSpace"));
    QVERIFY(backspace);

    outputs_.clear();
    machine_.press(*backspace);
    QTest::qWait(120);
    machine_.release(*backspace);
    const int count = outputs_.size();
    QVERIFY(count >= 3);

    QTest::qWait(60);
    QCOMPARE(outputs_.size(), count); // release stops the repeat
    machine_.setRepeatTiming(450, 55);
}

void TestKeyState::languageSwitch()
{
    machine_.setLayoutId(QStringLiteral("us"));
    machine_.setZenkakuOnLangSwitch(true);
    outputs_.clear();
    machine_.cycleLayout();

    QCOMPARE(machine_.layoutId(), QStringLiteral("jp106"));
    QVERIFY(!outputs_.isEmpty());
    const Output out = outputs_.last();
    QCOMPARE(out.setLanguage, QStringLiteral("jp106"));
    QCOMPARE(out.script.size(), 1);
    QCOMPARE(out.script.first().keysym, keysym("Zenkaku_Hankaku"));

    machine_.setZenkakuOnLangSwitch(false);
    machine_.setLayoutId(QStringLiteral("us"));
    QCOMPARE(machine_.layoutId(), QStringLiteral("us"));
}

QTEST_GUILESS_MAIN(TestKeyState)
#include "tst_keystate.moc"
