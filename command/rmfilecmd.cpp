#include "rmfilecmd.h"
#include <QJsonDocument>S

RmFileCmd::RmFileCmd(QString path_, QObject *parent) : QObject(parent), path(path_)
{
}

QString RmFileCmd::to_json() const
{
    QJsonObject command;
    command["command"] = "rm_file";
    command["path"] = path;
    return QString(QJsonDocument(command).toJson(QJsonDocument::Compact));
}
