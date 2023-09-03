#include "connection.h"

#include <QThread>

Connection::Connection(QObject *parent, QString url_) : QObject{parent}, idle(false), url(QUrl::fromUserInput(url_))
{
    qDebug() << this << " busy";
    connect(&socket, &QWebSocket::connected, this, &Connection::on_connected);
    connect(&socket, &QWebSocket::errorOccurred, this, &Connection::error);
    connect(&socket, &QWebSocket::stateChanged, this, &Connection::state_changed);
    url.setScheme("ws");
    socket.open(url);
}

Connection::~Connection()
{
    socket.abort();
}

bool Connection::is_idle() const
{
    return idle.load();
}

void Connection::send(QString command)
{
    idle.exchange(false);
    qDebug() << this << " busy";
    qDebug() << this << " sending command " << command;
    socket.sendTextMessage(command);
}

void Connection::error(QAbstractSocket::SocketError error)
{
    qDebug() << this << " error: " << error;
}

void Connection::state_changed(QAbstractSocket::SocketState state)
{
    qDebug() << this << " state changed: " << state;
}

void Connection::on_connected()
{
    qDebug() << this << " connected";
    connect(&socket, &QWebSocket::textMessageReceived, this, &Connection::on_text_message);
    qDebug() << this << " idle";
    idle.exchange(true);
}

void Connection::on_text_message(QString message)
{
    qDebug() << this << " idle";
    QString debug = message;
    if(debug.length() > 100)
    {
        debug.truncate(100);
    }
    qDebug() << this << " response: " << debug;
    emit response(message);
    idle.exchange(true);
}
