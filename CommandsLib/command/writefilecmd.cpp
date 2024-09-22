#include "writefilecmd.h"
#include <QJsonDocument>

WriteFileCmd::WriteFileCmd(QString path_, QString buf_, size_t size_, qint64 off_, QObject *parent) : QObject(parent), path(path_), buf(buf_), size(size_), off(off_)
{
}

QString WriteFileCmd::to_json() const
{
    QJsonObject command;
    command["command"] = "write_file";
    command["path"] = path;
    command["buf"] = buf;
    command["size"] = (qint64)size;
    command["offset"] = (qint64)off;
    return QString(QJsonDocument(command).toJson(QJsonDocument::Compact));
}
