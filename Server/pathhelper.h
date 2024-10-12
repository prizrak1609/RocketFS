#pragma once

#include <QObject>
#include <QMap>
#include <QDir>
#include <QString>

class PathHelper : public QObject
{
    Q_OBJECT
public:
    explicit PathHelper(QObject *parent = nullptr);

    QString findPath(QString path);
    void removePath(QString path);
    QStringList pathParts(QString path);
    bool contains(QDir dir, QString name, Qt::CaseSensitivity sensitive = Qt::CaseInsensitive);
    QString getOriginalName(QDir dir, QString name, Qt::CaseSensitivity sensitive = Qt::CaseInsensitive);
signals:

private:
    QMap<QString, QString> paths;
};
