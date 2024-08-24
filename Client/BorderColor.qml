import QtQuick 2.15

Rectangle {
    anchors.fill: parent

    z: -1

    radius: 10

    property int borderWidth: 1
    property color borderColor: "#fff"

    onBorderWidthChanged: {
        anchors.topMargin = -borderWidth
        anchors.bottomMargin = -borderWidth
        anchors.leftMargin = -borderWidth
        anchors.rightMargin = -borderWidth
    }

    onBorderColorChanged: {
        color = borderColor
    }
}
