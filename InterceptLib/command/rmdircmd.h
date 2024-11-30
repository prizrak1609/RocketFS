#pragma once

#include "ICommand.h"
#include <QObject>

class RmDirCmd : public QObject, public Command::ICommand
{
    Q_OBJECT
public:
    explicit RmDirCmd(QString path_, QObject *parent = nullptr);

    // ICommand interface
public:
    QString to_json() const override;

private:
    QString path;
};

