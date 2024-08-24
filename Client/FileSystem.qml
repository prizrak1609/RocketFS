import QtQuick 2.15
import QtQuick.Controls
import FileSystemModel 1.0

ListView {
    visible: true
    clip: true

    implicitWidth: parent.width
    implicitHeight: parent.height

    ScrollBar.vertical.interactive: true

    FileSystemDataSource {
        id: dataSource
        server: Server
        folderPath: "/"
    }

    model: dataSource

    delegate: Text {
        width: parent.width
        text: model.name

        TapHandler {
            onTapped: {
                dataSource.folderPath += "/" + text
            }
        }
    }
}
