#include "connection_pool.h"
#include <QFuture>

Connection_pool::Connection_pool(QObject *parent, QString url_) : QObject{parent}, url(url_)
{
    int count  = QThread::idealThreadCount();
    count = 1;
    for(int i = 0; i < count; i++)
    {
        pool.push_back(new Connection(this, url));
    }
}

Connection_pool::~Connection_pool()
{
    for(Connection* conn : pool)
    {
        delete conn;
    }
}

Connection* Connection_pool::get_connection()
{
    while(true)
    {
        QList<Connection*>::Iterator iter = pool.begin();
        for(; iter != pool.end(); iter++)
        {
            if ((*iter)->is_idle())
            {
                return *iter;
            }
        }
        qDebug() << "waiting";
        QThread::yieldCurrentThread();
        QThread::sleep(1);
    }
}
