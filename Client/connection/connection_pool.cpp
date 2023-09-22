#include "connection_pool.h"
#include <QFuture>
#include <QMutexLocker>

constexpr int kWaitTimeSec = 1;

Connection_pool::ptr Connection_pool::instance = {};

Connection_pool::Connection_pool(QObject *parent, QString url_) : QObject{parent}, url(url_)
{
    int count  = QThread::idealThreadCount();
    count = 1;
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

Connection_pool::ptr& Connection_pool::init(QObject *parent, QString url_)
{
    static std::once_flag flag;
    std::call_once(flag, [=](){
        instance.reset(new Connection_pool(parent, url_));
    });
    return instance;
}

Connection_pool::ptr& Connection_pool::get_instance()
{
    return instance;
}

Connection* Connection_pool::get_connection()
{
    Connection* res = nullptr;
    while(true)
    {
        {
            QMutexLocker<QMutex> lock(&send_mutex);
            for(Connection* conn : idle)
            {
                if (conn->is_idle())
                {
                    res = conn;
                    goto prepare_lists;
                }
                qDebug() << conn << "waiting for result:" << conn->get_last_command();
            }
        }
        QThread::yieldCurrentThread();
        QThread::sleep(kWaitTimeSec);
    }

    prepare_lists:

    {
        QMutexLocker<QMutex> lock(&send_mutex);
        idle.removeOne(res);
        busy.push_back(res);
    }

    return res;
}

QFuture<QString> Connection_pool::send_text(ICommand &command)
{
    Connection* conn = get_connection();

    connect(this, &Connection_pool::request, conn, &Connection::send, Qt::SingleShotConnection);

    QFuture<QString> result = QtFuture::connect(conn, &Connection::response_string);

    emit request(command.to_json());

    return result.then(QtFuture::Launch::Sync, [this, conn](QString message) -> QString {
        QMutexLocker<QMutex> lock(&send_mutex);
        busy.removeOne(conn);
        idle.push_back(conn);
        return message;
    });
}

QFuture<QByteArray> Connection_pool::send_binary(ICommand &command)
{
    Connection* conn = get_connection();

    connect(this, &Connection_pool::request, conn, &Connection::send, Qt::SingleShotConnection);

    QFuture<QByteArray> result = QtFuture::connect(conn, &Connection::response_bytes);

    emit request(command.to_json());

    return result.then(QtFuture::Launch::Sync, [this, conn](QByteArray message) -> QByteArray {
        QMutexLocker<QMutex> lock(&send_mutex);
        busy.removeOne(conn);
        idle.push_back(conn);
        return message;
    });
}
