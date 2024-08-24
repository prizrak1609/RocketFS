#pragma once

#include "ICommand.h"
#include <QObject>

class ReadDirCmd : public QObject, public WebSocket::Command::ICommand
{
    Q_OBJECT
public:
    explicit ReadDirCmd(QString path, QObject *parent = nullptr);

    // ICommand interface
public:
    QString to_json() const override;

private:
    QString _path;
};

