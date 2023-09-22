#pragma once

#include <QMainWindow>
#include "connection_pool.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void connect();
    void text_error(QString);
    void socket_error(QAbstractSocket::SocketError);

private:
    Ui::MainWindow *ui;
};
