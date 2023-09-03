#include "createfilecmd.h"
#include <QJsonDocument>

CreateFileCmd::CreateFileCmd(QString path_, QObject *parent) : QObject(parent), path(path_)
{
}

QString CreateFileCmd::to_json() const
{
    QJsonObject command;
    command["command"] = "create_file";
    command["path"] = path;
    return QString(QJsonDocument(command).toJson(QJsonDocument::Compact));
}
