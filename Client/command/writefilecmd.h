#pragma once

#include "ICommand.h"
#include <QObject>

using namespace WebSocket::Command;

class WriteFileCmd : public QObject, public ICommand
{
    Q_OBJECT
public:
    explicit WriteFileCmd(QString path_, QString buf_, size_t size_, qint64 off_, QObject *parent = nullptr);

    // ICommand interface
public:
    QString to_json() const override;

private:
    QString path;
    QString buf;
    size_t size;
    qint64 off;
};

