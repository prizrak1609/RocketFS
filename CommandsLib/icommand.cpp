#include "ICommand.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "command/getattrcmd.h"
#include "command/readdircmd.h"
#include "command/mkdircmd.h"
#include "command/rmdircmd.h"
#include "command/renamecmd.h"
#include "command/createfilecmd.h"
#include "command/rmfilecmd.h"
#include "command/openfilecmd.h"
#include "command/readfilecmd.h"
#include "command/writefilecmd.h"
#include "command/closefilecmd.h"
#include "command/statfscmd.h"

using namespace Command;

ICommand* ICommand::fromJson(QObject* parent, QString json) {
    qDebug() << "received: " << json;
    QJsonObject obj = QJsonDocument::fromJson(json.toUtf8()).object();
    QString command = obj["command"].toString();
    if(command == "get_attr")
    {
        QString path = obj["path"].toString();
        qDebug() << "get_attr: " << path;
        return new GetAttrCmd(path, parent);
    } else if(command == "read_dir")
    {
        QString path = obj["path"].toString();
        qDebug() << "read_dir: " << path;
        return new ReadDirCmd(path, parent);
    } else if(command == "mk_dir")
    {
        QString path = obj["path"].toString();
        qDebug() << "mk_dir: " << path;
        return new MkDirCmd(path, parent);
    } else if(command == "rm_dir")
    {
        QString path = obj["path"].toString();
        qDebug() << "rm_dir: " << path;
        return new RmDirCmd(path, parent);
    } else if(command == "rename")
    {
        QString from = obj["from"].toString();
        QString to = obj["to"].toString();
        qDebug() << "rename: from " << from << " to " << to;
        return new RenameCmd(from, to, parent);
    } else if(command == "create_file")
    {
        QString path = obj["path"].toString();
        qDebug() << "create_file: " << path;
        return new CreateFileCmd(path, parent);
    } else if(command == "rm_file")
    {
        QString path = obj["path"].toString();
        qDebug() << "rm_file: " << path;
        return new RmFileCmd(path, parent);
    } else if(command == "open_file")
    {
        QString path = obj["path"].toString();
        qDebug() << "open_file: " << path;
        return new OpenFileCmd(path, parent);
    } else if(command == "read_file")
    {
        QString path = obj["path"].toString();
        int64_t size = obj["size"].toInteger();
        int64_t off = obj["offset"].toInteger();
        qDebug() << "read_file: " << path <<" size " << size << " offset " << off;
        return new ReadFileCmd(path, size, off, parent);
    } else if(command == "write_file")
    {
        QString path = obj["path"].toString();
        QString buf = obj["buf"].toString();
        int64_t size = obj["size"].toInteger();
        int64_t off = obj["offset"].toInteger();
        qDebug() << "write_file: " << path <<" size " << size << " offset " << off;
        return new WriteFileCmd(path, buf, size, off, parent);
    } else if(command == "close_file")
    {
        QString path = obj["path"].toString();
        qDebug() << "close_file: " << path;
        return new CloseFileCmd(path, parent);
    } else if(command == "stat_fs")
    {
        QString path = obj["path"].toString();
        qDebug() << "stat_fs: " << path;
        return new StatFSCmd(path, parent);
    }
    return nullptr;
}
