#include "connection.h"

#include <QThread>
#include <QMutexLocker>

Connection::Connection(QObject *parent, QString url_) : QObject{parent}, locker(new QMutex()), url(QUrl::fromUserInput(url_))
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
    return !locker.isLocked();
}

QString Connection::get_last_command() const
{
    return last_command;
}

void Connection::send(QString command)
{
    while (locker.isLocked())
    {
        QThread::yieldCurrentThread();
        QThread::sleep(1);
        qDebug() << this << "waiting";
    }

    locker.relock();

    last_command = command;
    qDebug() << this << "busy";
    qDebug() << this << "sending command" << command;
    socket.sendTextMessage(command);
}

void Connection::error(QAbstractSocket::SocketError error)
{
    qDebug() << this << "error:" << error;
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
    qDebug() << this << "idle";

    locker.unlock();
}

void Connection::on_text_message(QString message)
{
    QString debug = message;
    if(debug.length() > 100)
    {
        debug.truncate(100);
    }
    qDebug() << this << "response:" << debug;

    locker.unlock();

    emit response_string(message);
    qDebug() << this << "idle";
}

void Connection::on_binary_message(QByteArray message)
{
    qDebug() << this << "response:" << message.size();

    locker.unlock();

    emit response_bytes(message);
    qDebug() << this << "idle";
}
