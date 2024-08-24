#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connection_pool.h"
#include "readdircmd.h"

using namespace WebSocket;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(ui->connect_button, &QPushButton::clicked, this, &MainWindow::connect);
    QObject::connect(ui->refresh_button, &QPushButton::clicked, this, &MainWindow::refresh);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connect()
{
    qDebug() << ui->server_addr->text() << "\n";

    Connection_pool::init(this, ui->server_addr->text());
    QObject::connect(Connection_pool::get_instance().get(), &Connection_pool::error, this, &MainWindow::text_error);

    ui->status->setText("Started");

    QObject::disconnect(ui->connect_button, &QPushButton::clicked, this, &MainWindow::connect);
}

void MainWindow::refresh()
{
    ui->status->setText("Rereshing");

    ReadDirCmd cmd("/", this);
    Connection_pool::get_instance()->send_text(cmd).then([](QString text){

    });
}

void MainWindow::text_error(QString message)
{
    ui->error->setText("error: " + message);
}
