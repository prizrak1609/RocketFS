#pragma once

#include <QMainWindow>
#include "filesystem/filesystem.h"
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
    void filesystem_error(QString);

private:
    Ui::MainWindow *ui;
    Filesystem* filesystem;
    Connection_pool* pool;
};
