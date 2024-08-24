#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QQmlEngine>
#include <QJsonObject>
#include <QList>
#include <QStringList>
#include "server.h"

class FileSystemDataSource : public QStringList, QObject
{
    Q_OBJECT

    Q_PROPERTY(QString folderPath READ folderPath WRITE setFolderPath NOTIFY folderPathChanged FINAL)
    Q_PROPERTY(Server* server READ server WRITE setServer NOTIFY serverChanged FINAL)

    QML_ELEMENT

    struct ItemData {
        QString name;
        QJsonObject stats;
    };

public:
    explicit FileSystemDataSource(QObject *parent = nullptr);

    void fetchMore(const QModelIndex &parent) ;
    bool canFetchMore(const QModelIndex &parent) const ;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const ;

    QString folderPath();
    void setFolderPath(QString path);
    Server* server();
    void setServer(Server* server);

signals:
    void folderPathChanged(QString);
    void serverChanged(Server*);

protected slots:
    void resetInternalData();

private:
    QString _folderPath;
    Server* _server;
    QList<ItemData> _sources;

    void fetchData();
};
