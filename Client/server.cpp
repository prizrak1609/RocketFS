#include "server.h"
#include <QDebug>
#include "connection_pool.h"
#include "command/readdircmd.h"

using namespace WebSocket;

Server::Server(QObject *parent) : QObject{parent}
{}

QFuture<QString> Server::readDir(QString path)
{
    ReadDirCmd cmd(path, this);
    return Connection_pool::get_instance()->send_text(cmd);
}

void Server::connect(QString address) {
    qDebug() << address;

    Connection_pool::init(this, address);
    QObject::connect(Connection_pool::get_instance(), &Connection_pool::error, this, &Server::error);

    emit connected();
}

void Server::error(QString text) {
    qDebug() << "error: " << text;
}
