#include "connection.h"

#include <QThread>
#include <QMutexLocker>

using namespace WebSocket;

Connection::Connection(QObject *parent, QString url) : QObject{parent}, _idle{false}, _url(QUrl::fromUserInput(url))
{
    connect(&_socket, &QWebSocket::connected, this, &Connection::on_connected);
    connect(&_socket, &QWebSocket::errorOccurred, this, &Connection::error);
    connect(&_socket, &QWebSocket::stateChanged, this, &Connection::state_changed);
    _url.setScheme("ws");
    qDebug() << this << " connection opens " << _url;
    _socket.open(_url);
}

Connection::~Connection()
{
    _socket.abort();
}

bool Connection::is_idle() const
{
    return _idle.load();
}

QString Connection::get_last_command() const
{
    return _last_command;
}

void Connection::send(QString command)
{
    _idle.exchange(false);

    _last_command = command;

    // qDebug() << this << "sending command" << command;

    _socket.sendTextMessage(command);
}

void Connection::on_error(QAbstractSocket::SocketError error_)
{
    // qDebug() << this << "error:" << error_;

    emit error(error_);

    _idle.exchange(true);
}

void Connection::state_changed(QAbstractSocket::SocketState state)
{
    qDebug() << this << "state changed:" << state;
}

void Connection::on_connected()
{
    qDebug() << this << "connected";
    connect(&_socket, &QWebSocket::textMessageReceived, this, &Connection::on_text_message);
    connect(&_socket, &QWebSocket::binaryMessageReceived, this, &Connection::on_binary_message);

    _idle.exchange(true);
}

void Connection::on_text_message(QString message)
{
    QString debug = message;
    if(debug.length() > 100)
    {
        debug.truncate(100);
    }
    // qDebug() << this << "response:" << debug;

    emit response_string(message);

    _idle.exchange(true);
}

void Connection::on_binary_message(QByteArray message)
{
    // qDebug() << this << "response:" << message.size();

    emit response_bytes(message);

    _idle.exchange(true);
}
