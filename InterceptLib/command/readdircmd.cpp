#include "readdircmd.h"
#include <QJsonDocument>

ReadDirCmd::ReadDirCmd(QString path, QObject *parent) : _path(path)
{
}

QString ReadDirCmd::to_json() const
{
    QJsonObject command;
    command["command"] = "read_dir";
    command["path"] = _path;
    return QString(QJsonDocument(command).toJson(QJsonDocument::Compact));
}
