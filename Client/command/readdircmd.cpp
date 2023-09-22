#include "readdircmd.h"
#include <QJsonDocument>

ReadDirCmd::ReadDirCmd(QString path_, QObject *parent) : path(path_)
{
}

QString ReadDirCmd::to_json() const
{
    QJsonObject command;
    command["command"] = "read_dir";
    command["path"] = path;
    return QString(QJsonDocument(command).toJson(QJsonDocument::Compact));
}
