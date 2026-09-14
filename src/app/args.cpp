#include "app/args.h"

#include "app/limits.h"
#include "core/blocks.h"

namespace osk {

bool parseCommandLine(const QStringList &args, CommandLineOptions *out, QString *error)
{
    for (int i = 0; i < args.size(); ++i) {
        const QString arg = args.at(i);
        const int eq = arg.indexOf(QLatin1Char('='));
        const QString name = eq < 0 ? arg : arg.left(eq);
        QString value = eq < 0 ? QString() : arg.mid(eq + 1);

        const auto fail = [error](const QString &message) {
            if (error)
                *error = message;
            return false;
        };
        // Options that take a value accept "--name VALUE" and "--name=VALUE".
        const auto takeValue = [&]() -> bool {
            if (eq >= 0) {
                if (value.isEmpty())
                    return fail(QStringLiteral("option %1 needs a value").arg(name));
                return true;
            }
            if (i + 1 >= args.size())
                return fail(QStringLiteral("option %1 needs a value").arg(name));
            value = args.at(++i);
            return true;
        };

        if (name == QLatin1String("--show") || name == QLatin1String("--hide")
            || name == QLatin1String("--toggle") || name == QLatin1String("--dark")
            || name == QLatin1String("--light")) {
            if (eq >= 0)
                return fail(QStringLiteral("option %1 does not take a value").arg(name));
            if (name == QLatin1String("--show"))
                out->show = true;
            else if (name == QLatin1String("--hide"))
                out->hide = true;
            else if (name == QLatin1String("--toggle"))
                out->toggle = true;
            else if (name == QLatin1String("--dark"))
                out->dark = true;
            else
                out->light = true;
        } else if (name == QLatin1String("--mode")) {
            if (!takeValue())
                return false;
            out->mode = value;
        } else if (name == QLatin1String("--lang")) {
            if (!takeValue())
                return false;
            out->language = value;
        } else if (name == QLatin1String("--theme")) {
            if (!takeValue())
                return false;
            out->theme = value;
        } else if (name == QLatin1String("--blocks")) {
            if (!takeValue())
                return false;
            const QStringList ids = value.split(QLatin1Char(','), Qt::SkipEmptyParts);
            if (ids.isEmpty())
                return fail(QStringLiteral("option --blocks needs at least one block id"));
            for (const QString &id : ids) {
                if (id != QLatin1String(blocks::kFrow) && id != QLatin1String(blocks::kNumpad)) {
                    return fail(QStringLiteral("unknown block \"%1\" (known: %2, %3)")
                                        .arg(id, QLatin1String(blocks::kFrow), QLatin1String(blocks::kNumpad)));
                }
                out->blocks.insert(id);
            }
        } else if (name == QLatin1String("--scale")) {
            if (!takeValue())
                return false;
            bool converted = false;
            const double scale = value.toDouble(&converted);
            if (!converted || scale < limits::minScale || scale > limits::maxScale) {
                return fail(QStringLiteral("invalid --scale \"%1\" (expected %2 - %3)")
                                    .arg(value)
                                    .arg(limits::minScale)
                                    .arg(limits::maxScale));
            }
            out->scale = scale;
        } else {
            return fail(QStringLiteral("unknown option \"%1\"").arg(name));
        }
    }
    return true;
}

} // namespace osk
