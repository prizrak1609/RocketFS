#pragma once

#include "ICommand.h"
#include <QObject>

class ReadDirCmd : public QObject, public ICommand
{
    Q_OBJECT
public:
    explicit ReadDirCmd(QString path_, QObject *parent = nullptr);

    // ICommand interface
public:
    QString to_json() const override;

private:
    QString path;
};

