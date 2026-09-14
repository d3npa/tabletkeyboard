#include "app/singleinstance.h"

#include <QLocalServer>
#include <QLocalSocket>

namespace osk {

namespace {

QString socketName()
{
    return QStringLiteral("tabletkeyboard");
}

} // namespace

SingleInstance::SingleInstance(QObject *parent) : QObject(parent) {}

SingleInstance::ClaimResult SingleInstance::claim(QString *error)
{
    QLocalSocket probe;
    probe.connectToServer(socketName());
    if (probe.waitForConnected(200)) {
        probe.disconnectFromServer();
        return Secondary;
    }

    QLocalServer::removeServer(socketName());
    server_ = new QLocalServer(this);
    if (!server_->listen(socketName())) {
        if (error)
            *error = QStringLiteral("cannot create the single-instance socket: %1").arg(server_->errorString());
        return Failed;
    }

    connect(server_, &QLocalServer::newConnection, this, [this]() {
        while (QLocalSocket *socket = server_->nextPendingConnection()) {
            connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
            connect(socket, &QLocalSocket::readyRead, socket, [this, socket]() {
                const QString payload = QString::fromUtf8(socket->readAll());
                socket->disconnectFromServer();
                QStringList args = payload.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                if (!args.isEmpty())
                    emit commandReceived(args);
            });
        }
    });

    return Primary;
}

bool SingleInstance::forward(const QStringList &args) const
{
    QLocalSocket socket;
    socket.connectToServer(socketName());
    if (!socket.waitForConnected(500))
        return false;
    socket.write(args.join(QLatin1Char('\n')).toUtf8());
    if (!socket.waitForBytesWritten(500))
        return false;
    socket.disconnectFromServer();
    return true;
}

} // namespace osk
