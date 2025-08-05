#include "server.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QMutexLocker>
#include <QJsonArray>
#include <sys/stat.h>
#include <QStorageInfo>
#include <QMetaEnum>
#include <thread>
#include <chrono>

constexpr auto kExecutableExtensions = {".APP", ".BAT", ".BIN", ".CAB", ".COM", ".CMD", ".COMMAND", ".CPL", ".CSH", ".EX_", ".EXE", ".GADGET", ".INF", ".INS", ".INX", ".ISU", ".JOB",
                                        ".JSE", ".KSH", ".LNK", ".MSC", ".MSI", ".MSP", ".MST", ".OSX", ".OUT", ".PAF", ".PIF", ".PS1", ".REG", ".RGS", ".RUN", ".SCR", ".SCT",
                                        ".SHB", ".SHS", ".U3P", ".VB", ".VBE", ".VBS", ".VBSCRIPT", ".WORKFLOW", ".WS", ".WSF", ".WSH", ".DLL"};
constexpr int kBlockSize = 4096;

Server::Server(QObject *parent) : QObject{parent}, web_socket_server(new QWebSocketServer("File transfer", QWebSocketServer::NonSecureMode, this))
{
    if(web_socket_server->listen(QHostAddress::Any, 8091))
    {
        qDebug() << "listening on " << web_socket_server->serverUrl();
        QObject::connect(web_socket_server, &QWebSocketServer::newConnection, this, &Server::new_connection);
        QObject::connect(web_socket_server, &QWebSocketServer::acceptError, this, &Server::acceptError);
        QObject::connect(web_socket_server, &QWebSocketServer::serverError, this, &Server::serverError);
    }
}

void Server::acceptError(QAbstractSocket::SocketError err) {
    qDebug() << QString("accept error: ").append(QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(err));
}

void Server::serverError(QWebSocketProtocol::CloseCode err) {
    qDebug() << QString("server error: ").append(QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(err));
}

Server::~Server()
{
    qDebug() << "closing connections";
    web_socket_server->close();
}

void Server::new_connection()
{
    QWebSocket* socket = web_socket_server->nextPendingConnection();
    qDebug() << "new connection " << socket << " port " << socket->localPort();
    QObject::connect(socket, &QWebSocket::textMessageReceived, this, &Server::handle_text_message);
    QObject::connect(socket, &QWebSocket::disconnected, this, &Server::disconnected);
}

void Server::handle_text_message(QString message)
{
    bool delayIsSet = false;
    int delay = qEnvironmentVariableIntValue("TEST_DELAY_MS", &delayIsSet);
    if (delayIsSet) {
        qDebug() << "test: delaying message for " << delay << "milliseconds";
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
    uint64_t messageNumber = counter.fetch_add(1);
    qDebug() << messageNumber << ": " << qobject_cast<QWebSocket *>(sender()) << "received: " << message;
    QJsonObject obj = QJsonDocument::fromJson(message.toUtf8()).object();
    QString command = obj["command"].toString();
    QString result = "";
    if(command == "get_attr")
    {
        QString path = obj["path"].toString();
        qDebug() << "get_attr: " << path;
        result = get_attr(path);
    } else if(command == "read_dir")
    {
        QString path = obj["path"].toString();
        qDebug() << "read_dir: " << path;
        result = read_dir(path);
    } else if(command == "mk_dir")
    {
        QString path = obj["path"].toString();
        qDebug() << "mk_dir: " << path;
        result = mk_dir(path);
    } else if(command == "rm_dir")
    {
        QString path = obj["path"].toString();
        qDebug() << "rm_dir: " << path;
        result = rm_dir(path);
    } else if(command == "rename")
    {
        QString from = obj["from"].toString();
        QString to = obj["to"].toString();
        qDebug() << "rename: from " << from << " to " << to;
        result = rename(from, to);
    } else if(command == "create_file")
    {
        QString path = obj["path"].toString();
        qDebug() << "create_file: " << path;
        result = create_file(path);
    } else if(command == "rm_file")
    {
        QString path = obj["path"].toString();
        qDebug() << "rm_file: " << path;
        result = rm_file(path);
    } else if(command == "open_file")
    {
        QString path = obj["path"].toString();
        qDebug() << "open_file: " << path;
        result = open_file(path);
    } else if(command == "read_file")
    {
        QString path = obj["path"].toString();
        int64_t size = obj["size"].toInteger();
        int64_t off = obj["offset"].toInteger();
        qDebug() << "read_file: " << path <<" size " << size << " offset " << off;
        result = read_file(path, size, off);
        // read file sends binary
        return;
    } else if(command == "write_file")
    {
        QString path = obj["path"].toString();
        QString buf = obj["buf"].toString();
        int64_t size = obj["size"].toInteger();
        int64_t off = obj["offset"].toInteger();
        qDebug() << "write_file: " << path <<" size " << size << " offset " << off;
        result = write_file(path, buf, size, off);
    } else if(command == "close_file")
    {
        QString path = obj["path"].toString();
        qDebug() << "close_file: " << path;
        result = close_file(path);
    } else if(command == "stat_fs")
    {
        QString path = obj["path"].toString();
        qDebug() << "stat_fs: " << path;
        result = stat_fs(path);
    }
    qDebug() << messageNumber << ": " << qobject_cast<QWebSocket *>(sender()) << " result: " << result;
    qobject_cast<QWebSocket *>(sender())->sendTextMessage(result);
}

void Server::disconnected()
{
    QWebSocket *client = qobject_cast<QWebSocket *>(sender());

    QObject::disconnect(client, &QWebSocket::textMessageReceived, this, &Server::handle_text_message);
    QObject::disconnect(client, &QWebSocket::disconnected, this, &Server::disconnected);

    client->close();
    client->deleteLater();
    qDebug() << "disconnected: " << client;
}

QString Server::get_attr(QString path)
{
    QString originalPath = path_helper.findPath(path);
    if (originalPath.isEmpty())
    {
        return "";
    }

    QFileInfo file(originalPath);
    if (file.exists())
    {
        QJsonObject response = stat_to_json(file);
        return QJsonDocument(response).toJson(QJsonDocument::Compact);
    }
    qDebug() << "get_attr: not found";
    return "";
}

QString Server::read_dir(QString path)
{
    QString originalPath = path_helper.findPath(path);
    if (originalPath.isEmpty())
    {
        return "";
    }

    QJsonArray result;
    QDir dir(originalPath);
    for (const QString& item : dir.entryList())
    {
        if (item == ".") {
            continue;
        }

        QString item_full_path = path + dir.separator() + item;

        QFileInfo file(item_full_path);

        QJsonObject entry;
        entry["file_name"] = item;
        entry["stats"] = stat_to_json(file);

        result.append(entry);
    }

    qDebug() << "read_dir: response";
    return QJsonDocument(result).toJson(QJsonDocument::Compact);
}

QString Server::mk_dir(QString path)
{
    QDir dir;
    dir.mkpath(path);
    return "";
}

QString Server::rm_dir(QString path)
{
    path_helper.removePath(path);

    QString originalPath = path_helper.findPath(path);
    if (originalPath.isEmpty())
    {
        return "";
    }

    QDir dir(originalPath);
    dir.removeRecursively();
    return "";
}

QString Server::rename(QString from, QString to)
{
    path_helper.removePath(from);

    QString originalPath = path_helper.findPath(from);
    if (originalPath.isEmpty())
    {
        return "";
    }

    QFileInfo info(originalPath);
    if(info.isFile())
    {
        QFile file;
        file.rename(originalPath, to);
    } else
    {
        QDir dir;
        dir.rename(originalPath, to);
    }
    return "";
}

QString Server::create_file(QString path)
{
    QFile file(path);
    file.open(QFile::WriteOnly);
    file.flush();
    file.close();
    return "";
}

QString Server::rm_file(QString path)
{
    path_helper.removePath(path);

    QString originalPath = path_helper.findPath(path);
    if (originalPath.isEmpty())
    {
        return "";
    }

    QFile file(originalPath);
    file.remove();

    return "";
}

QString Server::open_file(QString path)
{
    qDebug() << "open_file: unsupported";
    return "";
}

QString Server::read_file(QString path, int64_t size, int64_t off)
{
    QString originalPath = path_helper.findPath(path);
    if (originalPath.isEmpty())
    {
        qDebug() << "read_file: not exist, empty response";
        qobject_cast<QWebSocket *>(sender())->sendBinaryMessage(QByteArray());
        return "";
    }

    QFile file(originalPath);
    if (file.exists() && file.open(QFile::ReadWrite) && file.seek(off))
    {
        QByteArray buf = file.read(size);

        qDebug() << "read_file: response " << buf.size();
        qobject_cast<QWebSocket *>(sender())->sendBinaryMessage(buf);
        return "";
    }

    qDebug() << "read_file: empty response";
    qobject_cast<QWebSocket *>(sender())->sendBinaryMessage(QByteArray());
    return "";
}

QString Server::write_file(QString path, QString buf, int64_t size, int64_t off)
{
    QString originalPath = path_helper.findPath(path);
    if (originalPath.isEmpty())
    {
        return "";
    }

    QFile file(originalPath);
    if (file.exists())
    {
        file.open(QFile::ReadWrite);

        QDataStream stream(&file);
        stream.skipRawData(off);

        stream.writeRawData(QByteArray::fromBase64(buf.toUtf8(), QByteArray::Base64UrlEncoding), buf.size());

        file.flush();
    }
    return "";
}

QString Server::close_file(QString path)
{
    qDebug() << "close_file: unsupported";
    return "";
}

QString Server::stat_fs(QString path)
{
    QString originalPath = path_helper.findPath(path);
    if (originalPath.isEmpty())
    {
        originalPath = QDir::rootPath();
    }

    QStorageInfo storage = QStorageInfo::root();
    storage.setPath(path);

    QJsonObject body;
    body["block_size"] = storage.blockSize();
    body["free_size"] = storage.bytesFree();
    body["blocks_count"] = storage.bytesTotal() / storage.blockSize();
    body["blocks_free_count"] = storage.bytesFree() / storage.blockSize();
    body["blocks_available_count"] = storage.bytesAvailable() / storage.blockSize();

    return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

QJsonObject Server::stat_to_json(const QFileInfo& info)
{
    struct stat file_stat;
    stat(info.absoluteFilePath().toStdString().c_str(), &file_stat);

    QJsonObject result;
    result["st_dev"] = (qint64)file_stat.st_dev;
    result["st_ino"] = (qint64)file_stat.st_ino;

    int mode = 0;
    if (info.isDir())
    {
        qDebug() << info.absoluteFilePath() << " is directory";
        mode = S_IFDIR + 0777;
    } else
    {
        qDebug() << info.absoluteFilePath() << " is file";
        mode = S_IFREG;

        if (info.isReadable())
        {
            qDebug() << info.absoluteFilePath() << " readable";
            mode += 0444;
        }
        if (info.isWritable())
        {
            qDebug() << info.absoluteFilePath() << " writable";
            mode += 0222;
        }

        bool is_executable = info.isExecutable();
        if (!is_executable)
        {
            for (const QString& ext : kExecutableExtensions)
            {
                if (info.fileName().endsWith(ext, Qt::CaseInsensitive))
                {
                    is_executable = true;
                    break;
                }
            }
        }
        if (is_executable)
        {
            qDebug() << info.path() << " executable";
            mode += 0111;
        }
    }

    result["st_mode"] = mode;
    result["st_nlink"] = (qint64)file_stat.st_nlink;
    result["st_size"] = (qint64)file_stat.st_size;
    result["st_blksize"] = kBlockSize;
    result["st_blocks"] = (qint64)(file_stat.st_size + kBlockSize - 1) / kBlockSize;
    result["st_atime_tv_sec"] = 0;
    result["st_atime_tv_nsec"] = (qint64)file_stat.st_atime;
    result["st_mtim_tv_sec"] = 0;
    result["st_mtim_tv_nsec"] = (qint64)file_stat.st_mtime;
    result["st_ctim_tv_sec"] = 0;
    result["st_ctim_tv_nsec"] = (qint64)file_stat.st_ctime;

    return result;
}
