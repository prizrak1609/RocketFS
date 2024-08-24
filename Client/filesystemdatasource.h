#pragma once

#include <QAbstractListModel>
#include <QJsonObject>
#include "server.h"

class FileSystemDataSource : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString folderPath READ folderPath WRITE setFolderPath NOTIFY folderPathChanged FINAL)
    Q_PROPERTY(Server* server READ server WRITE setServer NOTIFY serverChanged FINAL)

    struct ItemData {
        QString name;
        QJsonObject stats;
    };

    enum Roles {
        NameRole = Qt::UserRole
    };

public:
    explicit FileSystemDataSource(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int,QByteArray> roleNames() const override;

    QString folderPath();
    void setFolderPath(QString path);
    Server* server();
    void setServer(Server* server);

signals:
    void folderPathChanged(QString);
    void serverChanged(Server*);

private:
    QString _folderPath;
    Server* _server;
    QList<ItemData> _sources;

    void fetchData();
};
