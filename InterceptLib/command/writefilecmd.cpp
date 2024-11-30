#include "writefilecmd.h"
#include <QJsonDocument>

WriteFileCmd::WriteFileCmd(QString path_, QByteArray buf_, size_t size_, qint64 off_, QObject *parent) : QObject(parent), path(path_), buf(buf_), size(size_), off(off_)
{
}

QString WriteFileCmd::to_json() const
{
    QJsonObject command;
    command["command"] = "write_file";
    command["path"] = path;
    command["buf"] = QString(buf.toBase64(QByteArray::Base64UrlEncoding));
    command["size"] = (qint64)size;
    command["offset"] = (qint64)off;
    return QJsonDocument(command).toJson(QJsonDocument::Compact);
}
