#include "rmdircmd.h"

#include <QJsonDocument>

RmDirCmd::RmDirCmd(QString path_, QObject *parent) : QObject(parent), path(path_)
{
}

QString RmDirCmd::to_json() const
{
    QJsonObject command;
    command["command"] = "rm_dir";
    command["path"] = path;
    return QString(QJsonDocument(command).toJson(QJsonDocument::Compact));
}
