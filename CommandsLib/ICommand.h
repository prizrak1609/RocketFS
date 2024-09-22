#pragma once

#include <QJsonObject>
#include <QObject>

namespace Command {

struct ICommand {
    virtual QString to_json() const = 0;
    virtual ~ICommand() = default;

    static ICommand* fromJson(QObject* parent, QString json);
};

}
