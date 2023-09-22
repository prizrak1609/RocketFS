#include "connection.h"

#include <QThread>
#include <QMutexLocker>

Connection::Connection(QObject *parent, QString url_) : QObject{parent}, idle{false}, url(QUrl::fromUserInput(url_))
{
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

QString Connection::get_last_command() const
{
    return last_command;
}

void Connection::send(QString command)
{
    idle.exchange(false);

    last_command = command;

    qDebug() << this << "sending command" << command;

    socket.sendTextMessage(command);
}

void Connection::error(QAbstractSocket::SocketError error)
{
    qDebug() << this << "error:" << error;
    idle.exchange(true);
}

void Connection::state_changed(QAbstractSocket::SocketState state)
{
    qDebug() << this << "state changed:" << state;
}

void Connection::on_connected()
{
    qDebug() << this << "connected";
    connect(&socket, &QWebSocket::textMessageReceived, this, &Connection::on_text_message);
    connect(&socket, &QWebSocket::binaryMessageReceived, this, &Connection::on_binary_message);

    idle.exchange(true);
}

void Connection::on_text_message(QString message)
{
    QString debug = message;
    if(debug.length() > 100)
    {
        debug.truncate(100);
    }
    qDebug() << this << "response:" << debug;

    emit response_string(message);

    idle.exchange(true);
}

void Connection::on_binary_message(QByteArray message)
{
    qDebug() << this << "response:" << message.size();

    emit response_bytes(message);

    idle.exchange(true);
}
