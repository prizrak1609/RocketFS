#pragma once

#include "ICommand.h"
#include <QObject>

using namespace WebSocket::Command;

class StatFSCmd : public QObject, public ICommand
{
    Q_OBJECT
public:
    explicit StatFSCmd(QString path_, QObject *parent = nullptr);

    // ICommand interface
public:
    QString to_json() const override;

private:
    QString path;
};

