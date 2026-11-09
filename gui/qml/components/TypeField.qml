import QtQuick
FocusScope {
    id: root
    property alias text: edit.text
    property string placeholder: "Path / filename"
    property color paper: "#F2EDE4"
    property color ink: "#14110F"
    property color accent: "#E8472B"
    signal accepted(string value)
    implicitHeight: 44
    Rectangle {anchors.fill:parent; color: Qt.rgba(root.ink.r,root.ink.g,root.ink.b,.025)
        border.color: root.activeFocus ? root.accent : Qt.rgba(root.ink.r,root.ink.g,root.ink.b,.25)
        border.width: root.activeFocus ? 2 : 1}
    TextInput {
        id: edit
        anchors.fill: parent; anchors.leftMargin: 11; anchors.rightMargin: 11
        verticalAlignment: TextInput.AlignVCenter
        font.family: "JetBrains Mono"; font.pixelSize: 12
        color: root.ink; selectionColor: root.accent; selectedTextColor: "white"
        clip: true; activeFocusOnTab: true
        onAccepted: root.accepted(text)
    }
    Text {anchors.fill:edit; verticalAlignment:Text.AlignVCenter
        color: root.ink; opacity:.42; text: root.placeholder
        font.family:"JetBrains Mono"; font.pixelSize:12
        visible: edit.text.length===0 && !edit.activeFocus }
}
