#include "filesystemdatasource.h"
#include <QJsonDocument>
#include <QJsonArray>

FileSystemDataSource::FileSystemDataSource(QObject *parent)
    : QAbstractListModel(parent)
{}

int FileSystemDataSource::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return _sources.size();
}

QVariant FileSystemDataSource::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    return _sources[index.row()].name;
}

QHash<int, QByteArray> FileSystemDataSource::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    return roles;
}

QString FileSystemDataSource::folderPath()
{
    return _folderPath;
}

void FileSystemDataSource::setFolderPath(QString path) {
    _folderPath = path;
    _sources.clear();

    fetchData();

    emit folderPathChanged(_folderPath);
}

Server* FileSystemDataSource::server() {
    return _server;
}

void FileSystemDataSource::setServer(Server* server) {
    _server = server;
    emit serverChanged(_server);
}

void FileSystemDataSource::fetchData() {
    _server->readDir(_folderPath).then(QtFuture::Launch::Async, [this](QString text) {
        QJsonDocument document = QJsonDocument::fromJson(text.toUtf8());
        QJsonArray array = document.array();
        for (const QJsonValue& val : std::as_const(array)) {
            QJsonObject obj = val.toObject();
            ItemData item;
            item.name = obj["file_name"].toString();
            item.stats = obj["stats"].toObject();
            _sources.push_back(qMove(item));
        }
    });
}
