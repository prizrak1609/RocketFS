#include "getattrcmd.h"
#include <QJsonDocument>

GetAttrCmd::GetAttrCmd(QString path_, QObject *parent) : QObject{parent}, path(path_)
{
}

QString GetAttrCmd::to_json() const
{
    QJsonObject command;
    command["command"] = "get_attr";
    command["path"] = path;
    return QString(QJsonDocument(command).toJson(QJsonDocument::Compact));
}
