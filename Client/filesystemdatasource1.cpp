#include "filesystemdatasource1.h"
#include <QJsonDocument>
#include <QJsonArray>
#include "readdircmd.h"
#include "connection_pool.h"

using namespace WebSocket;

FileSystemDataSource::FileSystemDataSource(QObject *parent) : QObject{parent}
{}

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
    _server->readDir(_folderPath).then(QtFuture::Launch::Sync, [this](QString text) {
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

bool FileSystemDataSource::canFetchMore(const QModelIndex &parent) const {
    return parent.row() < _sources.size();
}

QVariant FileSystemDataSource::data(const QModelIndex &index, int role) const {
    return _sources[index.row()].name;
}

void FileSystemDataSource::fetchMore(const QModelIndex &parent) {
    fetchData();
}

void FileSystemDataSource::resetInternalData() {
    _sources.clear();
}
