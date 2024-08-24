#pragma once

#include <QObject>
#include <qqmlintegration.h>
#include <QFuture>

class Server : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

public:
    explicit Server(QObject *parent = nullptr);

    QFuture<QString> readDir(QString path);

signals:
    void connected();

public slots:
    void connect(QString address);

private slots:
    void error(QString text);
};
