#pragma once

#include <QObject>
#include <QtWebSockets/QWebSocketServer>
#include <QWebSocket>
#include <QMutex>
#include <QFile>
#include <qfileinfo.h>
#include <atomic>
#include "pathhelper.h"

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    ~Server();

public slots:
    void new_connection();
    void handle_text_message(QString message);
    void disconnected();
    void acceptError(QAbstractSocket::SocketError socketError);
    void serverError(QWebSocketProtocol::CloseCode closeCode);

private:
    PathHelper path_helper;
    std::atomic_uint64_t counter = 0;
    QWebSocketServer* web_socket_server;
    QList<QWebSocket*> clients;
    
    QString get_attr(QString path);
    QString read_dir(QString path);
    QString mk_dir(QString path);
    QString rm_dir(QString path);
    QString rename(QString from, QString to);
    QString create_file(QString path);
    QString rm_file(QString path);
    QString open_file(QString path);
    QString read_file(QString path, int64_t size, int64_t off);
    QString write_file(QString path, QString buf, int64_t size, int64_t off);
    QString close_file(QString path);
    
    QJsonObject stat_to_json(const QFileInfo& info);
    QString stat_fs(QString path);
};

