#pragma once

#include "ICommand.h"
#include <QObject>

class GetAttrCmd : public QObject, public ICommand
{
    Q_OBJECT
public:
    explicit GetAttrCmd(QString path_, QObject *parent = nullptr);

    // ICommand interface
public:
    QString to_json() const override;

private:
    QString path;
};

