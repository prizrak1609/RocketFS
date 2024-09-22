#include "connection_pool.h"
#include <QFuture>
#include <QMutexLocker>
#include <QMetaEnum>

using namespace WebSocket;

constexpr int kWaitTimeSec = 500;

Connection_pool* Connection_pool::_instance = nullptr;

Connection_pool::Connection_pool(QObject *parent, QString url) : QObject{parent}, _url(url)
{
    int count  = QThread::idealThreadCount();
    for(int i = 0; i < count; i++)
    {
        Connection* conn = new Connection(this, url);
        connect(conn, &Connection::error, this, &Connection_pool::on_error);
        _idle.push_back(conn);
    }
}

Connection_pool::~Connection_pool()
{
    qDeleteAll(_idle);
    qDeleteAll(_busy);
}

Connection_pool* Connection_pool::init(QObject *parent, QString url)
{
    delete _instance;
    _instance = new Connection_pool(parent, url);
    return _instance;
}

Connection_pool* Connection_pool::get_instance()
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
        QThread::msleep(kWaitTimeSec);
    }

    prepare_lists:

    {
        QMutexLocker<QMutex> lock(&_send_mutex);
        _idle.removeOne(res);
        _busy.push_back(res);
    }

    return res;
}

QFuture<QString> Connection_pool::send_text(Command::ICommand &command)
{
    Connection* conn = get_connection();

    QObject::connect(this, SIGNAL(request), conn, SLOT(send), Qt::SingleShotConnection);

    QFuture<QString> result = QtFuture::connect(conn, &Connection::response_string);

    emit request(command);

    return result.then(QtFuture::Launch::Sync, [this, conn](QString message) -> QString {
        QMutexLocker<QMutex> lock(&_send_mutex);
        _busy.removeOne(conn);
        _idle.push_back(conn);
        return message;
    });
}

QFuture<QByteArray> Connection_pool::send_binary(Command::ICommand &command)
{
    Connection* conn = get_connection();

    QObject::connect(this, SIGNAL(request), conn, SLOT(send), Qt::SingleShotConnection);

    QFuture<QByteArray> result = QtFuture::connect(conn, &Connection::response_bytes);

    emit request(command);

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
