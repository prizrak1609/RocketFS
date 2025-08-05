#pragma once

#include <QObject>
#include <QFuture>
#include <QThread>
#include <QDir>

class Filesystem : public QThread
{
    Q_OBJECT

    friend struct FileSystemImpl;
public:
    using ptr = std::unique_ptr<Filesystem>;

    ~Filesystem();

    static Filesystem::ptr& get_instance();

    QString cache_folder;
    QString mount_path;

signals:
    void error(QString);

private:
    static ptr instance;
    QDir cache;
    QMap<QString, QString> file_attributes;

    struct OpenedFile
    {
        QMutex* mutex = new QMutex();
        int links = 1;
    };

    QMap<QString, OpenedFile> opened_files;

    Filesystem(QObject *parent = nullptr);

    QFuture<QString> readDir(QString path);

    QString cache_path(const char *path);

    // QThread interface
protected:
    void run() override;
};
