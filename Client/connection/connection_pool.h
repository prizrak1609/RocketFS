#pragma once

#include "connection.h"
#include <QObject>
#include <QFuture>
#include <QList>
#include <QMutex>
#include <atomic>
#include "ICommand.h"

class Connection_pool : public QObject
{
    Q_OBJECT
public:
    using ptr = std::unique_ptr<Connection_pool>;
    ~Connection_pool();

    static ptr& init(QObject *parent = nullptr, QString url_ = "");
    static ptr& get_instance();

    QFuture<QString> send_text(ICommand& command);
    QFuture<QByteArray> send_binary(ICommand& command);

signals:
    void request(QString command);
    void error(QAbstractSocket::SocketError);

public slots:
    void on_error(QAbstractSocket::SocketError);

private:
    static ptr instance;
    QList<Connection*> idle;
    QList<Connection*> busy;
    QString url;
    QMutex send_mutex;

    Connection_pool(QObject *parent = nullptr, QString url_ = "");

    Connection* get_connection();
};
