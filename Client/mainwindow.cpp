#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connection_pool.h"
#include "readdircmd.h"
#include <QJsonArray>
#include <QJsonDocument>


using namespace WebSocket;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(ui->connect_button, &QPushButton::clicked, this, &MainWindow::connect);
    QObject::connect(ui->read_folder_button, &QPushButton::clicked, this, &MainWindow::read_folder);
    QObject::connect(ui->filesystem_list, &QListWidget::itemDoubleClicked, this, &MainWindow::open_item);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connect()
{
    qDebug() << ui->server_addr->text() << "\n";

    Connection_pool::init(this, ui->server_addr->text());
    QObject::connect(Connection_pool::get_instance(), &Connection_pool::error, this, &MainWindow::text_error);

    ui->status->setText("Started");

    QObject::disconnect(ui->connect_button, &QPushButton::clicked, this, &MainWindow::connect);
}

void MainWindow::read_folder()
{
    ui->status->setText("Reading " + ui->file_path->text());

    ui->filesystem_list->clear();

    ReadDirCmd cmd(ui->file_path->text(), this);
    Connection_pool::get_instance()->send_text(cmd).then(QtFuture::Launch::Sync, [this](QString text) {
        QJsonDocument document = QJsonDocument::fromJson(text.toUtf8());
        QJsonArray array = document.array();
        for (const QJsonValue& val : std::as_const(array)) {
            ui->filesystem_list->addItem(val.toObject()["file_name"].toString());
        }
    });
}

void MainWindow::open_item(QListWidgetItem *item)
{
    ui->file_path->setText(ui->file_path->text() + "/" + item->text());

    read_folder();
}

void MainWindow::text_error(QString message)
{
    ui->error->setText("error: " + message);
}
