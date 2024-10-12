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
    if (paths.contains(path))
    {
        return paths[path];
    }

    QFileInfo info(path);
    if (info.exists())
    {
        paths[path] = path;
    } else
    {
        QString absolutePath = QFileInfo(QDir::cleanPath(path)).absoluteFilePath();
        QStringList parts = pathParts(absolutePath);
        QString originalPath = "";
        QDir root = QDir::root();
        for (const QString& part : parts)
        {
            QString originalName = getOriginalName(root, part);
            if (!originalName.isEmpty())
            {
                root.cd(originalName);
                originalPath.append(QDir::separator()).append(originalName);
            } else
            {
                return "";
            }
        }

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
    QDirIterator iter(dir);
    while (iter.hasNext())
    {
        if (iter.nextFileInfo().fileName().compare(name, sensitive))
        {
            return iter.nextFileInfo().fileName();
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
