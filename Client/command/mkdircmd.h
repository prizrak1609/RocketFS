#pragma once

#include "ICommand.h"
#include <QObject>

using namespace WebSocket::Command;

class MkDirCmd : public QObject, public ICommand
{
    Q_OBJECT
public:
    explicit MkDirCmd(QString path_, QObject *parent = nullptr);

// ICommand interface
public:
    QString to_json() const override;

private:
    QString path;
};

