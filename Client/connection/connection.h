#pragma once

#include <QObject>
#include <QUrl>
#include <QWebSocket>
#include <atomic>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>

class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection(QObject *parent = nullptr, QString url_ = "");
    ~Connection();

    bool is_idle() const;
    QString get_last_command() const;

signals:
    void response_string(QString);
    void response_bytes(QByteArray);

public slots:
    void send(QString command);
    void on_connected();
    void on_text_message(QString message);
    void on_binary_message(QByteArray message);
    void error(QAbstractSocket::SocketError error);
    void state_changed(QAbstractSocket::SocketState state);

private:
    std::atomic_bool idle;
    QUrl url;
    QWebSocket socket;
    QString last_command;
};
