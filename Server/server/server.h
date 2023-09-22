#pragma once

#include <QObject>
#include <QtWebSockets/QWebSocketServer>
#include <QWebSocket>
#include <QMutex>
#include <QFile>

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

private:
    QWebSocketServer* web_socket_server;
    QList<QWebSocket*> clients;

    struct OpenedFile
    {
        QFile* file = nullptr;
        int links = 0;
        QMutex* mutex = new QMutex();
    };

    QMap<QString, OpenedFile> opened_files;
    
    void get_attr(QString path);
    void read_dir(QString path);
    void mk_dir(QString path);
    void rm_dir(QString path);
    void rename(QString from, QString to);
    void create_file(QString path);
    void rm_file(QString path);
    void open_file(QString path);
    void read_file(QString path, int64_t size, int64_t off);
    void write_file(QString path, QString buf, int64_t size, int64_t off);
    void close_file(QString path);
    
    void stat_to_json(struct stat* file_stat, QJsonObject& result, bool is_dir);
    void stat_fs(QString path);
};

