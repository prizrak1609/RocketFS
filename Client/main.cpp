#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>
#include "filesystemdatasource.h"
#include "server.h"
#include "filesystem/filesystem.h"
#include "connection_pool.h"

using namespace WebSocket;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Fusion");

    qmlRegisterType<FileSystemDataSource>("FileSystemModel", 1, 0, "FileSystemDataSource");
    // qmlRegisterType<Server>("Server", 1, 0, "Server");
    Server server;
    qmlRegisterSingletonInstance("Server", 1, 0, "Server", &server);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() {
        QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.loadFromModule("qml.module", "MainWindow");

    // Connection_pool* pool = Connection_pool::init(nullptr, "192.168.0.14:8091");

    Filesystem::get_instance()->cache_folder = "F:/cache";
    Filesystem::get_instance()->mount_path = "Y:";
    Filesystem::get_instance()->start();

    // auto res = app.exec();

    // Filesystem::get_instance()->terminate();
    // pool->deleteLater();

    return app.exec();
}
