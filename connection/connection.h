#pragma once

#include <QObject>
#include <QUrl>
#include <QWebSocket>
#include <atomic>
#include <QThread>
#include "ICommand.h"

class Connection : public QObject
{
    Q_OBJECT
public:
    explicit Connection(QObject *parent = nullptr, QString url_ = "");
   ~Connection();

    bool is_idle() const;

signals:
    void response(QString);

public slots:
    void send(QString command);
    void on_connected();
    void on_text_message(QString message);
    void error(QAbstractSocket::SocketError error);
    void state_changed(QAbstractSocket::SocketState state);

private:
    std::atomic_bool idle;
    QUrl url;
    QWebSocket socket;
};
