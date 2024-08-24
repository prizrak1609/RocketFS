#include "connection_pool.h"
#include <QFuture>
#include <QMutexLocker>
#include <QMetaEnum>

using namespace WebSocket;
using namespace WebSocket::Command;

constexpr int kWaitTimeSec = 1;

Connection_pool::ptr Connection_pool::_instance = {};

Connection_pool::Connection_pool(QObject *parent, QString url) : QObject{parent}, _url(url)
{
    int count  = QThread::idealThreadCount();
    // count = 2; // TODO: remove this line after implementing multithreading
    for(int i = 0; i < count; i++)
    {
        Connection* conn = new Connection(this, url);
        connect(conn, &Connection::error, this, &Connection_pool::on_error);
        _idle.push_back(conn);
    }
}

Connection_pool::~Connection_pool()
{
    for(Connection* conn : _idle)
    {
        delete conn;
    }
    for(Connection* conn : _busy)
    {
        delete conn;
    }
}

Connection_pool::ptr& Connection_pool::init(QObject *parent, QString url)
{
    static std::once_flag flag;
    std::call_once(flag, [=](){
        _instance.reset(new Connection_pool(parent, url));
    });
    return _instance;
}

Connection_pool::ptr& Connection_pool::get_instance()
{
    return _instance;
}

Connection* Connection_pool::get_connection()
{
    Connection* res = nullptr;
    while(true)
    {
        {
            QMutexLocker<QMutex> lock(&_send_mutex);
            for(Connection* conn : _idle)
            {
                if (conn->is_idle())
                {
                    res = conn;
                    goto prepare_lists;
                }
                // qDebug() << conn << "waiting for result:" << conn->get_last_command();
            }
        }
        QThread::yieldCurrentThread();
        QThread::sleep(kWaitTimeSec);
    }

    prepare_lists:

    {
        QMutexLocker<QMutex> lock(&_send_mutex);
        _idle.removeOne(res);
        _busy.push_back(res);
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
        QMutexLocker<QMutex> lock(&_send_mutex);
        _busy.removeOne(conn);
        _idle.push_back(conn);
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
        QMutexLocker<QMutex> lock(&_send_mutex);
        _busy.removeOne(conn);
        _idle.push_back(conn);
        return message;
    });
}

void Connection_pool::on_error(QAbstractSocket::SocketError err)
{
    emit error(QString("error: ").append(QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(err)));
}
