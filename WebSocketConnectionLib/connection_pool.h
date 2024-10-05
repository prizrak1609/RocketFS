#pragma once

#include "connection.h"
#include <QObject>
#include <QFuture>
#include <QList>
#include <QMutex>
#include "ICommand.h"

namespace WebSocket {

    class Connection_pool : public QObject
    {
        Q_OBJECT
    public:
        ~Connection_pool();

        static Connection_pool* init(QObject *parent, QString url);
        static Connection_pool* get_instance();

        QFuture<QString> send_text(Command::ICommand& command);
        QFuture<QByteArray> send_binary(Command::ICommand& command);

    signals:
        void request(QString command);
        void error(QString error);

    public slots:
        void on_error(QAbstractSocket::SocketError);

    private:
        static Connection_pool* _instance;
        QList<Connection*> _idle;
        QList<Connection*> _busy;
        QString _url;
        QMutex _send_mutex;

        Connection_pool(QObject *parent, QString url);

        Connection* get_connection();
    };

}
