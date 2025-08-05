#include "filesystem.h"
#include <QDebug>

void Filesystem::run()
{
    qDebug() << "Start filesystem " << QThread::currentThreadId();
}
