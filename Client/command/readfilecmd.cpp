#include "readfilecmd.h"
#include <QJsonDocument>

ReadFileCmd::ReadFileCmd(QString path_, size_t size_, fuse_off_t off_, QObject *parent) : QObject(parent), path(path_), size(size_), off(off_)
{
}

QString ReadFileCmd::to_json() const
{
    QJsonObject command;
    command["command"] = "read_file";
    command["path"] = path;
    command["size"] = (qint64)size;
    command["offset"] = off;
    return QString(QJsonDocument(command).toJson(QJsonDocument::Compact));
}

