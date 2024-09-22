#include "mkdircmd.h"
#include <QJsonDocument>

MkDirCmd::MkDirCmd(QString path_, QObject *parent) : QObject(parent), path(path_)
{
}

QString MkDirCmd::to_json() const
{
    QJsonObject command;
    command["command"] = "mk_dir";
    command["path"] = path;
    return QString(QJsonDocument(command).toJson(QJsonDocument::Compact));
}
