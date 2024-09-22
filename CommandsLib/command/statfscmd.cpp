#include "statfscmd.h"
#include <QJsonDocument>

StatFSCmd::StatFSCmd(QString path_, QObject *parent) : QObject{parent}, path{path_}
{
}

QString StatFSCmd::to_json() const
{
    QJsonObject command;
    command["command"] = "stat_fs";
    command["path"] = path;
    return QString(QJsonDocument(command).toJson(QJsonDocument::Compact));
}
