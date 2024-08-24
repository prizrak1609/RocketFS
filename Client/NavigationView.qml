import QtQuick 2.15
import QtQuick.Controls

Rectangle {
    visible: true

    property alias startView: stack.initialItem

    StackView {
        id: stack
        anchors.fill: parent
    }

    function navigateToScreen(view) {
        view.visible = visible
        view.x = 0
        view.y = 0
        view.width = parent.width
        view.height = parent.height
        stack.push(view)
    }
}
