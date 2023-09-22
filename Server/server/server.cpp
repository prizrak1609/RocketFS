#include "server.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QMutexLocker>
#include <QJsonArray>
#include <sys/stat.h>

Server::Server(QObject *parent) : QObject{parent}, web_socket_server(new QWebSocketServer("File transfer", QWebSocketServer::NonSecureMode, this))
{
    if(web_socket_server->listen(QHostAddress::Any, 8091))
    {
        qDebug() << "listening on " << web_socket_server->serverPort() << " port";
        QObject::connect(web_socket_server, &QWebSocketServer::newConnection, this, &Server::new_connection);
    }
}

Server::~Server()
{
    qDebug() << "closing connections";
    web_socket_server->close();
    qDeleteAll(clients);
}

void Server::new_connection()
{
    QWebSocket* socket = web_socket_server->nextPendingConnection();
    qDebug() << "new connection " << socket;
    QObject::connect(socket, &QWebSocket::textMessageReceived, this, &Server::handle_text_message);
    QObject::connect(socket, &QWebSocket::disconnected, this, &Server::disconnected);
}

void Server::handle_text_message(QString message)
{
    qDebug() << "received: " << message;
    QJsonObject obj = QJsonDocument::fromJson(message.toUtf8()).object();
    QString command = obj["command"].toString();
    if(command == "get_attr")
    {
        QString path = obj["path"].toString();
        qDebug() << "get_attr: " << path;
        get_attr(path);
    } else if(command == "read_dir")
    {
        QString path = obj["path"].toString();
        qDebug() << "read_dir: " << path;
        read_dir(path);
    } else if(command == "mk_dir")
    {
        QString path = obj["path"].toString();
        qDebug() << "mk_dir: " << path;
        mk_dir(path);
    } else if(command == "rm_dir")
    {
        QString path = obj["path"].toString();
        qDebug() << "rm_dir: " << path;
        rm_dir(path);
    } else if(command == "rename")
    {
        QString from = obj["from"].toString();
        QString to = obj["to"].toString();
        qDebug() << "rename: from " << from << " to " << to;
        rename(from, to);
    } else if(command == "create_file")
    {
        QString path = obj["path"].toString();
        qDebug() << "create_file: " << path;
        create_file(path);
    } else if(command == "rm_file")
    {
        QString path = obj["path"].toString();
        qDebug() << "rm_file: " << path;
        rm_file(path);
    } else if(command == "open_file")
    {
        QString path = obj["path"].toString();
        qDebug() << "open_file: " << path;
        open_file(path);
    } else if(command == "read_file")
    {
        QString path = obj["path"].toString();
        int64_t size = obj["size"].toInteger();
        int64_t off = obj["offset"].toInteger();
        qDebug() << "read_file: " << path <<" size " << size << " offset " << off;
        read_file(path, size, off);
    } else if(command == "write_file")
    {
        QString path = obj["path"].toString();
        QString buf = obj["buf"].toString();
        int64_t size = obj["size"].toInteger();
        int64_t off = obj["offset"].toInteger();
        qDebug() << "write_file: " << path <<" size " << size << " offset " << off;
        write_file(path, buf, size, off);
    } else if(command == "close_file")
    {
        QString path = obj["path"].toString();
        qDebug() << "close_file: " << path;
        close_file(path);
    }
}

void Server::disconnected()
{
    QWebSocket *client = qobject_cast<QWebSocket *>(sender());
    clients.removeAll(client);
    client->deleteLater();
    qDebug() << "disconnected: " << client;
}

void Server::get_attr(QString path)
{
    QFileInfo file(path);
    if (file.exists())
    {
        struct stat* file_stat = new struct stat();
        stat(path.toStdString().c_str(), file_stat);
        QJsonObject response;
        stat_to_json(file_stat, response, file.isDir());

        qDebug() << "get_attr: " << path << "\nresponse: " << QJsonDocument(response).toJson(QJsonDocument::Compact);
        qobject_cast<QWebSocket *>(sender())->sendTextMessage(QString(QJsonDocument(response).toJson(QJsonDocument::Compact)));
        delete file_stat;
        return;
    }
    qDebug() << "get_attr: not found";
    qobject_cast<QWebSocket *>(sender())->sendTextMessage("");
}

void Server::read_dir(QString path)
{
    QJsonArray result;
    QDir dir(path);
    for(const QString& item : dir.entryList())
    {
        struct stat* file_stat = new struct stat();
        QString item_full_path = path + dir.separator() + item;
        stat(item_full_path.toStdString().c_str(), file_stat);

        QFileInfo file(item_full_path);
        QJsonObject stat;
        stat_to_json(file_stat, stat, file.isDir());

        QJsonObject entry;
        entry["file_name"] = item;
        entry["stats"] = stat;

        result.append(entry);
        delete file_stat;
    }

    qDebug() << "read_dir: response";
    qobject_cast<QWebSocket *>(sender())->sendTextMessage(QString(QJsonDocument(result).toJson(QJsonDocument::Compact)));
}

void Server::mk_dir(QString path)
{
    QDir dir;
    dir.mkpath(path);
    qDebug() << "mk_dir: response";
    qobject_cast<QWebSocket *>(sender())->sendTextMessage("");
}

void Server::rm_dir(QString path)
{
    QDir dir(path);
    dir.removeRecursively();
    qDebug() << "rm_dir: response";
    qobject_cast<QWebSocket *>(sender())->sendTextMessage("");
}

void Server::rename(QString from, QString to)
{
    QFileInfo info(from);
    if(info.isFile())
    {
        QFile file;
        file.rename(from, to);
    } else
    {
        QDir dir;
        dir.rename(from, to);
    }
    qDebug() << "rename: response";
    qobject_cast<QWebSocket *>(sender())->sendTextMessage("");
}

void Server::create_file(QString path)
{
    QFile file(path);
    file.open(QFile::OpenModeFlag::WriteOnly);
    file.close();
    qDebug() << "create_file: response";
    qobject_cast<QWebSocket *>(sender())->sendTextMessage("");
}

void Server::rm_file(QString path)
{
    QFile file(path);
    file.remove();
    qDebug() << "rm_file: response";
    qobject_cast<QWebSocket *>(sender())->sendTextMessage("");
}

void Server::open_file(QString path)
{
    if(opened_files.contains(path))
    {
        OpenedFile file = opened_files.value(path);
        QMutexLocker<QMutex> lock(file.mutex);
        file.links++;
        opened_files[path] = file;
    } else
    {
        if(QFileInfo::exists(path))
        {
            OpenedFile file;
            QMutexLocker<QMutex> lock(file.mutex);
            file.file = new QFile(path);
            file.file->open(QFile::ReadWrite);
            file.links = 1;
            opened_files.insert(path, file);
        }
    }
    qDebug() << "open_file: response";
    qobject_cast<QWebSocket *>(sender())->sendTextMessage("");
}

void Server::read_file(QString path, int64_t size, int64_t off)
{
    OpenedFile file = opened_files.value(path);
    QMutexLocker<QMutex> lock(file.mutex);
    file.file->seek(off);
    QByteArray buf = file.file->read(size);
    qDebug() << "read_file: response";
    qobject_cast<QWebSocket *>(sender())->sendBinaryMessage(buf);
}

void Server::write_file(QString path, QString buf, int64_t size, int64_t off)
{
    OpenedFile file = opened_files.value(path);
    QMutexLocker<QMutex> lock(file.mutex);
    file.file->seek(off);

    QString encoded = QByteArray::fromBase64(buf.toUtf8());
    file.file->write(encoded.toUtf8());
    qDebug() << "write_file: response";
    qobject_cast<QWebSocket *>(sender())->sendTextMessage("");
}

void Server::close_file(QString path)
{
    if(opened_files.contains(path))
    {
        OpenedFile file = opened_files.value(path);
        QMutexLocker<QMutex> lock(file.mutex);
        file.links--;
        if(file.links <= 0)
        {
            file.file->close();
            opened_files.remove(path);
        } else
        {
            opened_files[path] = file;
        }
    }
    qDebug() << "close_file: response";
    qobject_cast<QWebSocket *>(sender())->sendTextMessage("");
}

void Server::stat_to_json(struct stat* file_stat, QJsonObject& result, bool is_dir)
{
    result["st_dev"] = (qint64)file_stat->st_dev;
    result["st_ino"] = (qint64)file_stat->st_ino;
    if(is_dir)
    {
        result["st_mode"] = 16895;
    } else
    {
        result["st_mode"] = 33206;
    }
    result["st_nlink"] = (qint64)file_stat->st_nlink;
    result["st_size"] = (qint64)file_stat->st_size;
    result["st_blksize"] = 4096;
    result["st_blocks"] = (qint64)(file_stat->st_size + 4095) / 4096;
    result["st_atime_tv_sec"] = 0;
    result["st_atime_tv_nsec"] = (qint64)file_stat->st_atime;
    result["st_mtim_tv_sec"] = 0;
    result["st_mtim_tv_nsec"] = (qint64)file_stat->st_mtime;
    result["st_ctim_tv_sec"] = 0;
    result["st_ctim_tv_nsec"] = (qint64)file_stat->st_ctime;
}
