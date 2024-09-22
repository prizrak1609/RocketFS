#pragma once

#include "ICommand.h"
#include <QObject>

class RenameCmd : public QObject, public Command::ICommand
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

