#pragma once

#include "ICommand.h"
#include <QObject>

using namespace WebSocket::Command;

class RenameCmd : public QObject, public ICommand
{
    Q_OBJECT
public:
    explicit RenameCmd(QString from_, QString to_, QObject *parent = nullptr);

    // ICommand interface
public:
    QString to_json() const override;

private:
    QString from;
    QString to;
};

