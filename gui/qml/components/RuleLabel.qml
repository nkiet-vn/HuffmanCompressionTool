import QtQuick
Item {
    id: root
    property string text: "INDEX"
    property color ink: "#14110F"
    property color accent: "#E8472B"
    implicitHeight: 26
    Text {
        anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
        text: root.text; color: root.ink
        font.family: "IBM Plex Sans"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1.7
    }
    Rectangle {
        x: Math.min(parent.width*0.65, labelProbe.width+15); height: 1
        width: parent.width-x; anchors.verticalCenter: parent.verticalCenter
        color: root.ink; opacity: .2
    }
    Text {
        id: labelProbe; visible: false; text: root.text
        font.family: "IBM Plex Sans"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1.7
    }
}
