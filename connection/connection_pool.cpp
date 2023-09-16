#include "connection_pool.h"
#include <QFuture>
#include <QMutexLocker>

Connection_pool::Connection_pool(QObject *parent, QString url_) : QObject{parent}, url(url_)
{
    int count  = QThread::idealThreadCount();
//    count = 2;
    for(int i = 0; i < count; i++)
    {
        idle.push_back(new Connection(this, url));
    }
}

Connection_pool::~Connection_pool()
{
    for(Connection* conn : idle)
    {
        delete conn;
    }
    for(Connection* conn : busy)
    {
        delete conn;
    }
}

Connection* Connection_pool::get_connection()
{
    while(true)
    {
        for(Connection* conn : idle)
        {
            if (conn->is_idle())
            {
                return conn;
            }
            if (print_logs)
            qDebug() << conn << "waiting for result:" << conn->get_last_command();
        }
        if (print_logs)
        qDebug() << this << "waiting";
        QThread::yieldCurrentThread();
        QThread::sleep(1);
    }
}

QFuture<QString> Connection_pool::send_text(ICommand &command)
{
    Connection* conn = nullptr;
    {
        QMutexLocker<QMutex> lock(&send_mutex);

        conn = get_connection();

        idle.removeOne(conn);
        busy.push_back(conn);
    }

    connect(this, &Connection_pool::request, conn, &Connection::send, Qt::SingleShotConnection);

    QFuture<QString> result = QtFuture::connect(conn, &Connection::response_string);

    emit request(command.to_json());
    return result.then(QtFuture::Launch::Sync, [this, conn](QString message) -> QString {
        busy.removeOne(conn);
        idle.push_back(conn);
        return message;
    });
}

QFuture<QByteArray> Connection_pool::send_binary(ICommand &command)
{
    Connection* conn = nullptr;
    {
        QMutexLocker<QMutex> lock(&send_mutex);

        conn = get_connection();

        idle.removeOne(conn);
        busy.push_back(conn);
    }

    connect(this, &Connection_pool::request, conn, &Connection::send, Qt::SingleShotConnection);

    QFuture<QByteArray> result = QtFuture::connect(conn, &Connection::response_bytes);

    emit request(command.to_json());
    return result.then(QtFuture::Launch::Sync, [this, conn](QByteArray message) -> QByteArray {
        busy.removeOne(conn);
        idle.push_back(conn);
        return message;
    });
}

void Connection_pool::set_print_logs(bool print)
{
    print_logs = print;
    for(Connection* conn : idle)
    {
        conn->print_logs = print;
    }
    for(Connection* conn : busy)
    {
        conn->print_logs = print;
    }
}

