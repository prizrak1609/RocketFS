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

void Connection::on_text_message(const QString &message)
{
    QString debug = message;
    debug.truncate(100);
    // qDebug() << this << " text response: " << debug;

    QString _message = message;
    emit response_string(_message);

    _idle.exchange(true);
}

void Connection::on_binary_message(const QByteArray &message)
{
    // qDebug() << this << " binary response: " << message.size();

    QByteArray _message = qUncompress(message, 2);
    emit response_bytes(_message);

    _idle.exchange(true);
}
