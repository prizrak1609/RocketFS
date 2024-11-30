import QtQuick 2.15
import QtQuick.Controls
import ServerModule 1.0

ApplicationWindow {
    id: root
    visible: true

    maximumWidth: 800
    maximumHeight: 600
    minimumWidth: 800
    minimumHeight: 600

    title: qsTr("Remote FileSystem Client")

    NavigationView {
        id: navigate
        anchors.fill: parent

        startView: Connect {
            serverAddress: "192.168.0.14:8091"

            onConnectToServer: function(address) {
                Server.connect(address)
            }
        }
    }

    Connections {
        target: Server
        function onConnected() {
            navigate.navigateToScreen(Qt.createComponent("FileSystem.qml", root))
        }
    }
}
