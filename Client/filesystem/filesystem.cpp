#include "filesystem.h"
#include <QDebug>
#include "command/getattrcmd.h"
#include "command/readdircmd.h"
#include <QJsonDocument>
#include <QJsonArray>
#include "command/mkdircmd.h"
#include "command/rmdircmd.h"
#include "command/renamecmd.h"
#include "command/createfilecmd.h"
#include "command/rmfilecmd.h"
#include "command/readfilecmd.h"
#include "command/writefilecmd.h"
#include "command/statfscmd.h"
#include "connection_pool.h"

using namespace WebSocket;

Filesystem::ptr Filesystem::instance = {};

constexpr const char* kFileAttributesName = "file_attributes.json";

Filesystem::Filesystem(QObject *parent) : QThread{parent}
{
    connect(this, &QThread::finished, this, &QObject::deleteLater);
}

Filesystem::~Filesystem()
{
    for (OpenedFile& pair : opened_files.values())
    {
        delete pair.mutex;
    }

    quit();
}

Filesystem::ptr& Filesystem::get_instance()
{
    static std::once_flag flag;
    std::call_once(flag, [&]{
        Filesystem::instance.reset(new Filesystem());
    });
    return Filesystem::instance;
}

QString Filesystem::cache_path(const char* path)
{
    QString file_path = cache_folder;
    file_path += "\\" + QString(path).replace("/", "_").replace(" ", "_");
    return file_path;
}
