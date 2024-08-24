import QtQuick 2.15
import QtQuick.Controls

TextField {
    visible: true

    property string placeholder

    Text {
        visible: !parent.text
        anchors.centerIn: parent
        text: placeholder
        font: parent.font
        color: parent.color
    }
}
