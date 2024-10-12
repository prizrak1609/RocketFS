#include "pathhelper.h"
#include <QFileInfo>
#include <QDirIterator>

PathHelper::PathHelper(QObject *parent) : QObject{parent}
{}

QStringList PathHelper::pathParts(QString path)
{
    if (path.isEmpty())
    {
        return {};
    }

    return path.split(QDir::separator(), Qt::SkipEmptyParts, Qt::CaseInsensitive);
}

QString PathHelper::findPath(QString path)
{
    qDebug() << "find path " << path;
    if (paths.contains(path))
    {
        qDebug() << "find path cached return " << paths[path];
        return paths[path];
    }

    QFileInfo info(path);
    if (info.exists())
    {
        qDebug() << "find path exists return " << path;
        paths[path] = path;
    } else
    {
        QString absolutePath = QFileInfo(QDir::cleanPath(path)).absoluteFilePath();
        qDebug() << "absolute path " << absolutePath;
        QStringList parts = pathParts(absolutePath);
        qDebug() << "splitted path " << parts;
        QString originalPath = "";
        QDir root = QDir::root();
        for (const QString& part : parts)
        {
            QString originalName = getOriginalName(root, part);
            qDebug() << part << " original name " << originalName;
            if (!originalName.isEmpty())
            {
                root.cd(originalName);
                originalPath.append(QDir::separator()).append(originalName);
                qDebug() << "original path " << originalPath;
            } else
            {
                return "";
            }
        }

        qDebug() << "find path found " << originalPath;
        paths[path] = originalPath;
    }

    return paths[path];
}

bool PathHelper::contains(QDir dir, QString name, Qt::CaseSensitivity sensitive)
{
    QDirIterator iter(dir);
    while (iter.hasNext())
    {
        if (iter.nextFileInfo().fileName().compare(name, sensitive))
        {
            return true;
        }
    }

    return false;
}

QString PathHelper::getOriginalName(QDir dir, QString name, Qt::CaseSensitivity sensitive)
{
    for(const QString& item : dir.entryList())
    {
        if (item == "." || item == "..")
        {
            continue;
        }

        qDebug() << item << " compare " << name;
        if (item.compare(name, sensitive))
        {
            return item;
        }
    }

    return "";
}

void PathHelper::removePath(QString path)
{
    paths.remove(path);

    QString absolutePath = QFileInfo(QDir::cleanPath(path)).absoluteFilePath();
    QFileInfo info(absolutePath);
    if (info.isDir())
    {
        QDir dir(absolutePath);
        QDirIterator iter(dir, QDirIterator::Subdirectories);
        while (iter.hasNext())
        {
            paths.remove(QDir::cleanPath(iter.next()));
        }
    }
}
