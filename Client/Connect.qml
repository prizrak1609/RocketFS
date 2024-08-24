import QtQuick 2.15
import QtQuick.Controls

Rectangle {
    visible: true
    color: "#000"

    signal connectToServer(string address)
    property string serverAddress: ""

    TextInputField {
        id: serverAddressField

        visible: parent.visible

        anchors.centerIn: parent
        width: 300

        font.pointSize: 20

        placeholder: "Server address"

        color: "#fff"

        text: serverAddress

        background: Rectangle {
            color: "#000"
            radius: adressBorder.radius
        }

        horizontalAlignment: TextInput.AlignHCenter

        BorderColor {
            id: adressBorder
            borderWidth: 2
            borderColor: "#fff"
        }

        validator: RegularExpressionValidator {
            regularExpression: /[0-9]{0,3}\.[0-9]{0,3}\.[0-9]{0,3}\.[0-9]{0,3}:[0-9]{0,4}/
        }
    }

    Button {
        id: connectButton

        anchors.verticalCenter: parent.verticalCenter
        anchors.left: serverAddressField.right
        anchors.leftMargin: 10

        text: "Connect"

        background: Rectangle {
            color: "#000"
            radius: buttonBorder.radius
        }

        BorderColor {
            id: buttonBorder
            borderWidth: 2
            borderColor: "#fff"
        }

        onPressed: {
            background.color = "gray"
        }

        onReleased: {
            background.color = "#000"
        }

        onClicked: {
            if (serverAddressField.text.length > 0) {
                connectToServer(serverAddressField.text)
            }
        }
    }
}
