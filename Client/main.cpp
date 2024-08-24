#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>
#include "filesystemdatasource.h"
#include "server.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Fusion");

    qmlRegisterType<FileSystemDataSource>("FileSystemModel", 1, 0, "FileSystemDataSource");
    qmlRegisterType<Server>("Server", 1, 0, "Server");

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() {
        QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.loadFromModule("qml.module", "MainWindow");

    return app.exec();
}
