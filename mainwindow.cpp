#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connection_pool.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    filesystem = Filesystem::get_instance();

    QObject::connect(ui->connect_button, &QPushButton::clicked, this, &MainWindow::connect);
    QObject::connect(filesystem, &Filesystem::error, this, &MainWindow::filesystem_error);
}

MainWindow::~MainWindow()
{
    filesystem->terminate();
    delete ui;
}

void MainWindow::connect()
{
    qDebug() << ui->drive_letter->text() << "\n";
    qDebug() << ui->server_addr->text() << "\n";

    filesystem->cache_folder = ui->cache_folder->text();
    filesystem->pool = new Connection_pool(this, ui->server_addr->text());
    filesystem->mount_path = ui->drive_letter->text();
    filesystem->start();

    ui->status->setText("Started");
}

void MainWindow::filesystem_error(QString message)
{
    ui->error->setText("error: " + message);
}
