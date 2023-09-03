#pragma once

#include "connection.h"
#include <QObject>
#include <QFuture>
#include <QList>
#include <atomic>

class Connection_pool : public QObject
{
    Q_OBJECT
public:
    explicit Connection_pool(QObject *parent = nullptr, QString url_ = "");
    ~Connection_pool();

    Connection* get_connection();

public slots:
//    void init();

private:
    QList<Connection*> pool;
    QString url;
};
