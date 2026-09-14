// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 d3npa <gh@w1t.ch>
#include "app/app.h"
#include "app/args.h"
#include "core/lint.h"
#include "platform/x11/symresolver_x11.h"

#include <QApplication>
#include <QSocketNotifier>

#include <csignal>
#include <cstdio>
#include <sys/socket.h>
#include <unistd.h>

namespace {

int signalFds[2] = { -1, -1 };

void unixSignalHandler(int)
{
    if (signalFds[1] < 0)
        return;
    const char byte = 1;
    const ssize_t written = ::write(signalFds[1], &byte, 1);
    (void)written;
}

// SIGTERM/SIGINT/SIGHUP must run the normal quit path, otherwise remapped
// spare keycodes would be left behind on the X server.
void installSignalHandlers()
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, signalFds) != 0)
        return;

    auto *notifier = new QSocketNotifier(signalFds[0], QSocketNotifier::Read, qApp);
    QObject::connect(notifier, &QSocketNotifier::activated, qApp, []() {
        char byte = 0;
        const ssize_t read = ::read(signalFds[0], &byte, 1);
        (void)read;
        QCoreApplication::quit();
    });

    struct sigaction action {};
    action.sa_handler = unixSignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    ::sigaction(SIGINT, &action, nullptr);
    ::sigaction(SIGTERM, &action, nullptr);
    ::sigaction(SIGHUP, &action, nullptr);
}

void printUsage()
{
    std::printf("usage: tabletkeyboard [options]\n"
                "\n"
                "  --show                 show the keyboard\n"
                "  --hide                 hide the keyboard\n"
                "  --toggle               toggle the keyboard\n"
                "  --mode full|simple     switch keyboard mode\n"
                "  --lang ID              switch layout set (us, jp106, ...)\n"
                "  --theme ID             switch theme\n"
                "  --scale FACTOR         keyboard scale (0.5 - 3.0)\n"
                "  --dark | --light       dark or light theme for the keyboard\n"
                "  --blocks ID[,ID]       blocks to show: frow, numpad (others are always shown)\n"
                "  --check-layout FILE    lint a layout file and exit\n"
                "  --version              print the version and exit\n"
                "  --help                 this text\n");
}

int runCheckLayout(const QString &path)
{
    osk::XlibKeysymResolver resolver;
    const QVector<osk::LintIssue> issues = osk::Lint::checkLayoutFile(path, &resolver);
    for (const osk::LintIssue &issue : issues)
        std::printf("%s\n", qPrintable(issue.toString()));
    if (!osk::Lint::hasErrors(issues)) {
        std::printf("%s: ok\n", qPrintable(path));
        return 0;
    }
    return 1;
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("tabletkeyboard"));
    QCoreApplication::setOrganizationName(QStringLiteral("tabletkeyboard"));
    QCoreApplication::setApplicationVersion(QStringLiteral(TABLETKEYBOARD_VERSION));
    application.setQuitOnLastWindowClosed(false);

    const QStringList args = QCoreApplication::arguments().mid(1);

    if (args.contains(QStringLiteral("--version"))) {
        std::printf("tabletkeyboard %s\n", TABLETKEYBOARD_VERSION);
        return 0;
    }
    if (args.contains(QStringLiteral("--help")) || args.contains(QStringLiteral("-h"))) {
        printUsage();
        return 0;
    }

    const int checkIndex = args.indexOf(QStringLiteral("--check-layout"));
    if (checkIndex >= 0) {
        if (checkIndex + 1 >= args.size()) {
            std::fprintf(stderr, "tabletkeyboard: --check-layout needs a file\n");
            return 2;
        }
        return runCheckLayout(args.at(checkIndex + 1));
    }

    // Validate before starting (and before forwarding): a typo must not look
    // like a successful start.
    osk::CommandLineOptions options;
    QString error;
    if (!osk::parseCommandLine(args, &options, &error)) {
        std::fprintf(stderr, "tabletkeyboard: %s\nTry 'tabletkeyboard --help'.\n", qPrintable(error));
        return 2;
    }

    osk::App osk;
    switch (osk.init(&error)) {
    case osk::App::Secondary:
        osk.forwardArgs(args);
        return 0;
    case osk::App::Failed:
        std::fprintf(stderr, "tabletkeyboard: %s\n", qPrintable(error));
        return 1;
    case osk::App::Primary:
        break;
    }

    installSignalHandlers();

    osk.applyInitialSettings();
    osk.handleArgs(options);

    return application.exec();
}
