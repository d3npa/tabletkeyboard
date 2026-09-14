#pragma once

#include "core/layout.h"
#include "core/theme.h"

#include <QString>
#include <QVector>

namespace osk {

struct LintIssue
{
    enum Severity { Error, Warning };

    Severity severity = Error;
    QString file;
    QString path;
    QString message;

    QString toString() const;
};

// Structural validation of shipped/user data files. Keysym names are checked
// through the injected resolver so the core stays platform-free.
class Lint
{
public:
    static QVector<LintIssue> check(const LayoutSet &set, const KeysymResolver *resolver);
    static QVector<LintIssue> check(const ThemeSpec &theme);

    // Loads and lints one layout file; a load failure is reported as an error.
    static QVector<LintIssue> checkLayoutFile(const QString &path, const KeysymResolver *resolver);

    // Loads and lints one theme file; a load failure is reported as an error.
    static QVector<LintIssue> checkThemeFile(const QString &path);

    static bool hasErrors(const QVector<LintIssue> &issues);
};

} // namespace osk
