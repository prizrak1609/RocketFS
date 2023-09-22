#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connection/connection_pool.h"
#include "filesystem/filesystem.h"
#include <QMetaEnum>>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(ui->connect_button, &QPushButton::clicked, this, &MainWindow::connect);
    QObject::connect(Filesystem::get_instance().get(), &Filesystem::error, this, &MainWindow::text_error);
}

MainWindow::~MainWindow()
{
    Filesystem::get_instance()->terminate();
    delete ui;
}

void MainWindow::connect()
{
    qDebug() << ui->drive_letter->text() << "\n";
    qDebug() << ui->server_addr->text() << "\n";

    Connection_pool::init(this, ui->server_addr->text());
    QObject::connect(Connection_pool::get_instance().get(), &Connection_pool::error, this, &MainWindow::socket_error);

    Filesystem::get_instance()->cache_folder = ui->cache_folder->text();
    Filesystem::get_instance()->mount_path = ui->drive_letter->text();
    Filesystem::get_instance()->start();

    ui->status->setText("Started");
}

void MainWindow::text_error(QString message)
{
    ui->error->setText("error: " + message);
}

void MainWindow::socket_error(QAbstractSocket::SocketError message)
{
    ui->error->setText(QString("error: ").append(QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(message)));
}
