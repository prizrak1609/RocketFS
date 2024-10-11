#pragma once

#include <QObject>
#include <QUrl>
#include <QWebSocket>
#include <atomic>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>

namespace WebSocket {

    class Connection : public QObject
    {
        Q_OBJECT
    public:
        explicit Connection(QObject *parent, QString url);
        ~Connection();

        bool is_idle() const;
        QString get_last_command() const;

    signals:
        void response_string(QString);
        void response_bytes(QByteArray);
        void error(QAbstractSocket::SocketError);

    public slots:
        void send(QString command);

    private slots:
        void on_connected();
        void on_text_message(const QString &message);
        void on_binary_message(const QByteArray &message);
        void on_error(QAbstractSocket::SocketError error_);
        void state_changed(QAbstractSocket::SocketState state);

    private:
        std::atomic_bool _idle;
        QUrl _url;
        QWebSocket _socket;
        QString _last_command;
    };

}
