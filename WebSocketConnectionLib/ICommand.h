#pragma once

#include <QJsonObject>
#include <QObject>

namespace WebSocket {
    namespace Command {

        struct ICommand
        {
            virtual QString to_json() const = 0;
            virtual ~ICommand() = default;
        };

    }
}
