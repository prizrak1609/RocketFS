#include "renamecmd.h"

#include <QJsonDocument>

RenameCmd::RenameCmd(QString from_, QString to_, QObject *parent) : QObject(parent), from(from_), to(to_)
{
}

QString RenameCmd::to_json() const
{
    QJsonObject command;
    command["command"] = "rm_dir";
    command["from"] = from;
    command["to"] = to;
    return QString(QJsonDocument(command).toJson(QJsonDocument::Compact));
}
