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
    explicit Connection_pool(QObject *parent = nullptr, QString url_ = "");
    ~Connection_pool();

    Connection* get_connection();
    QFuture<QString> send_text(ICommand& command);
    QFuture<QByteArray> send_binary(ICommand& command);

public slots:
//    void init();

signals:
    void request(QString command);

private:
    QList<Connection*> idle;
    QList<Connection*> busy;
    QString url;
    QMutex send_mutex;
};
