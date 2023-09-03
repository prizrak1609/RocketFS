#pragma once

#include "ICommand.h"
#include <QObject>

class CreateFileCmd : public QObject, public ICommand
{
    Q_OBJECT
public:
    explicit CreateFileCmd(QString path_, QObject *parent = nullptr);

    // ICommand interface
public:
    QString to_json() const override;

private:
    QString path;
};

