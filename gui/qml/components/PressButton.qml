import QtQuick
FocusScope {
    id: root
    property string label: "ACTION"
    property bool primary: false
    property bool selected: false
    property bool compact: false
    property color paper: "#F2EDE4"
    property color ink: "#14110F"
    property color accent: "#E8472B"
    signal clicked()
    implicitHeight: compact ? 30 : 42
    implicitWidth: Math.max(compact ? 68 : 130, caption.implicitWidth + 25)
    activeFocusOnTab: enabled
    Keys.onReturnPressed: root.clicked()
    Keys.onEnterPressed: root.clicked()
    Keys.onSpacePressed: root.clicked()
    Rectangle {
        x: root.pressed ? 0 : 3; y: root.pressed ? 0 : 3
        width: root.width - 3; height: root.height - 3
        color: root.ink; visible: root.primary && root.enabled
    }
    Rectangle {
        id: surface; anchors.fill: parent; anchors.rightMargin: root.primary ? 3 : 0
        anchors.bottomMargin: root.primary ? 3 : 0
        x: root.pressed ? 3 : 0; y: root.pressed ? 3 : 0
        color: !root.enabled ? Qt.rgba(root.ink.r,root.ink.g,root.ink.b,.04) :
               root.primary ? root.accent : root.selected ? root.ink : "transparent"
        border.color: root.activeFocus ? root.accent : root.ink
        border.width: root.activeFocus ? 2 : 1
    }
    Text {
        id: caption; anchors.centerIn: surface; text: root.label
        font.family: "IBM Plex Sans"; font.pixelSize: root.compact ? 10 : 12; font.bold: true
        font.letterSpacing: 1.1
        color: root.primary || root.selected ? root.paper : root.ink
        opacity: root.enabled ? 1 : .38
    }
    property bool pressed: pointer.pressed
    MouseArea {
        id: pointer; anchors.fill: parent; cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        enabled: root.enabled
        onClicked: {root.forceActiveFocus(); root.clicked()}
    }
}
