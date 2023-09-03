#pragma once

#include <QJsonObject>
#include <QObject>

struct ICommand
{
    virtual QString to_json() const = 0;
    virtual ~ICommand() = default;
};
