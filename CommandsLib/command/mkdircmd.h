#pragma once

#include "ICommand.h"
#include <QObject>

class MkDirCmd : public QObject, public Command::ICommand
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

