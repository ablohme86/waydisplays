import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1120; height: 720; minimumWidth: 860; minimumHeight: 600
    visible: true; title: qsTr("VirtMonitors"); color: "#0b0f17"
    palette.window: "#0b0f17"; palette.windowText: "#edf2ff"; palette.base: "#121826"
    palette.text: "#edf2ff"; palette.button: "#202a3d"; palette.buttonText: "#edf2ff"
    palette.highlight: "#6c8cff"; palette.highlightedText: "#ffffff"

    component SoftButton: Button {
        id: control; property bool primary: false
        implicitHeight: 42; leftPadding: 18; rightPadding: 18
        background: Rectangle {
            radius: 10
            color: control.down ? (control.primary ? "#526fd1" : "#2b3750") : control.hovered ? (control.primary ? "#7896ff" : "#263249") : (control.primary ? "#6686f5" : "#1b2435")
            border.color: control.primary ? "transparent" : "#303c53"
        }
        contentItem: Text { text: control.text; color: "#f4f7ff"; font.pixelSize: 14; font.weight: Font.DemiBold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
    }

    component FormField: ColumnLayout {
        property alias label: caption.text; property alias text: input.text
        property alias validator: input.validator; property alias inputMethodHints: input.inputMethodHints
        Layout.fillWidth: true; spacing: 7
        Label { id: caption; color: "#9caac2"; font.pixelSize: 12; font.weight: Font.DemiBold }
        TextField {
            id: input; Layout.fillWidth: true; implicitHeight: 44; selectByMouse: true; color: "#f1f5ff"
            background: Rectangle { radius: 9; color: "#0d1320"; border.color: input.activeFocus ? "#6686f5" : "#2b3549" }
        }
    }

    header: ToolBar {
        implicitHeight: 76
        background: Rectangle { color: "#0e1420"; border.color: "#202a3a" }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 30; anchors.rightMargin: 30; spacing: 14
            Rectangle { width: 38; height: 38; radius: 11; color: "#6686f5"; Text { anchors.centerIn: parent; text: "▣"; color: "white"; font.pixelSize: 23; font.bold: true } }
            ColumnLayout { spacing: 0
                Label { text: "VirtMonitors"; color: "#f4f7ff"; font.pixelSize: 20; font.weight: Font.Bold }
                Label { text: "Virtuelle skjermer for KDE Wayland"; color: "#8492aa"; font.pixelSize: 12 }
            }
            Item { Layout.fillWidth: true }
            Rectangle { width: 9; height: 9; radius: 5; color: monitorManager.backendAvailable ? "#45d69a" : "#f4b860" }
            Label { text: monitorManager.backendDescription; color: "#aab6ca"; font.pixelSize: 13 }
            SoftButton { text: "+  Ny skjerm"; primary: true; onClicked: editor.openNew() }
        }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 30; spacing: 22
        RowLayout { Layout.fillWidth: true
            ColumnLayout { spacing: 4
                Label { text: "Skjermprofiler"; color: "#f4f7ff"; font.pixelSize: 27; font.weight: Font.Bold }
                Label { text: "Opprett, rediger og aktiver virtuelle monitorer for strømming."; color: "#8f9cb2"; font.pixelSize: 14 }
            }
            Item { Layout.fillWidth: true }
            SoftButton { text: "Start Sunshine på nytt"; enabled: !monitorManager.busy; onClicked: monitorManager.restartSunshine() }
            SoftButton { text: "↻  Oppdater"; onClicked: monitorManager.refresh() }
        }
        Rectangle {
            visible: !monitorManager.backendAvailable; Layout.fillWidth: true; implicitHeight: 58; radius: 11; color: "#2b2117"; border.color: "#66502e"
            Label { anchors.fill: parent; anchors.margins: 16; verticalAlignment: Text.AlignVCenter; text: "Backend mangler. Installer KRFB-pakken som inneholder krfb-virtualmonitor."; color: "#f4c979" }
        }
        ListView {
            id: profileList; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12; clip: true; model: monitorManager
            delegate: Rectangle {
                required property int index; required property string name; required property int widthPx; required property int heightPx
                required property int refreshHz; required property real displayScale; required property int port; required property bool active
                required property string outputName; required property bool sunshine
                width: profileList.width; height: 112; radius: 14; color: mouse.containsMouse ? "#171f2e" : "#121925"; border.color: active ? "#356d61" : "#263145"
                MouseArea { id: mouse; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                RowLayout {
                    anchors.fill: parent; anchors.margins: 18; spacing: 17
                    Rectangle {
                        width: 74; height: 60; radius: 7; color: "#0a101b"; border.color: active ? "#45d69a" : "#43506a"; border.width: 2
                        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.bottom; width: 24; height: 4; color: active ? "#45d69a" : "#43506a" }
                        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; anchors.top: parent.bottom; anchors.topMargin: 6; width: 40; height: 3; radius: 2; color: active ? "#45d69a" : "#43506a" }
                        Text { anchors.centerIn: parent; text: widthPx + "×\n" + heightPx; horizontalAlignment: Text.AlignHCenter; color: "#b9c5da"; font.pixelSize: 11 }
                    }
                    ColumnLayout { Layout.fillWidth: true; spacing: 6
                        RowLayout {
                            Label { text: name; color: "#f1f5ff"; font.pixelSize: 18; font.weight: Font.DemiBold }
                            Rectangle { width: stateText.implicitWidth + 16; height: 25; radius: 12; color: active ? "#173b34" : "#202838"; Label { id: stateText; anchors.centerIn: parent; text: active ? "AKTIV" : "STOPPET"; color: active ? "#58dda7" : "#8492aa"; font.pixelSize: 10; font.bold: true } }
                            Rectangle { visible: sunshine; width: sunText.implicitWidth + 16; height: 25; radius: 12; color: "#28243c"; Label { id: sunText; anchors.centerIn: parent; text: "SUNSHINE"; color: "#bdaaff"; font.pixelSize: 10; font.bold: true } }
                            Item { Layout.fillWidth: true }
                        }
                        Label { text: widthPx + " × " + heightPx + "  ·  " + displayScale + "× skalering  ·  port " + port; color: "#9aa7bc"; font.pixelSize: 13 }
                        Label { text: outputName; color: "#64738c"; font.pixelSize: 12; font.family: "monospace" }
                    }
                    SoftButton { text: "Rediger"; onClicked: editor.openEdit(index) }
                    SoftButton { text: active ? "Stopp" : "Start"; primary: !active; enabled: !monitorManager.busy; onClicked: active ? monitorManager.stopProfile(index) : monitorManager.startProfile(index) }
                }
            }
            footer: Item { width: 1; height: 8 }
            ScrollBar.vertical: ScrollBar {}
            Label { anchors.centerIn: parent; visible: profileList.count === 0; text: "Ingen skjermprofiler ennå\nTrykk «Ny skjerm» for å komme i gang"; color: "#75839a"; horizontalAlignment: Text.AlignHCenter; lineHeight: 1.5; font.pixelSize: 15 }
        }
        Rectangle { Layout.fillWidth: true; implicitHeight: 44; radius: 10; color: "#101722"; visible: monitorManager.statusMessage.length > 0
            Label { anchors.fill: parent; anchors.margins: 13; verticalAlignment: Text.AlignVCenter; text: monitorManager.statusMessage; color: "#aebbd0"; elide: Text.ElideRight }
        }
    }

    Dialog {
        id: editor; width: Math.min(650, window.width - 50); anchors.centerIn: parent; modal: true; dim: true; padding: 0
        property string profileId: ""; property string password: ""
        title: profileId.length ? "Rediger skjerm" : "Ny virtuell skjerm"
        function openNew() { profileId = ""; nameField.text = "Virtual-1920x1080"; widthField.text = "1920"; heightField.text = "1080"; scaleBox.currentIndex = 1; portField.text = "5901"; sunshineCheck.checked = true; password = ""; open() }
        function openEdit(row) { let p = monitorManager.profile(row); profileList.currentIndex = row; profileId = p.id; nameField.text = p.name; widthField.text = p.width; heightField.text = p.height; portField.text = p.port; sunshineCheck.checked = p.sunshine; password = p.password; let wanted = p.scale.toFixed(2); scaleBox.currentIndex = Math.max(0, scaleBox.model.indexOf(wanted)); open() }
        background: Rectangle { radius: 18; color: "#151c29"; border.color: "#313c50" }
        header: RowLayout { height: 72; spacing: 12
            Label { Layout.leftMargin: 24; text: editor.title; color: "#f3f6ff"; font.pixelSize: 21; font.weight: Font.Bold }
            Item { Layout.fillWidth: true }
            ToolButton { Layout.rightMargin: 16; text: "×"; font.pixelSize: 25; onClicked: editor.close() }
        }
        contentItem: ColumnLayout { spacing: 18
            Rectangle { Layout.fillWidth: true; height: 1; color: "#293347" }
            ColumnLayout { Layout.leftMargin: 24; Layout.rightMargin: 24; Layout.bottomMargin: 4; spacing: 16
                FormField { id: nameField; label: "Navn" }
                RowLayout { spacing: 14
                    FormField {
                        id: widthField
                        label: "Bredde (px)"
                        validator: IntValidator { bottom: 320; top: 16384 }
                        inputMethodHints: Qt.ImhDigitsOnly
                    }
                    FormField {
                        id: heightField
                        label: "Høyde (px)"
                        validator: IntValidator { bottom: 200; top: 8640 }
                        inputMethodHints: Qt.ImhDigitsOnly
                    }
                }
                RowLayout { spacing: 14
                    ColumnLayout { Layout.fillWidth: true; spacing: 7
                        Label { text: "Skalering"; color: "#9caac2"; font.pixelSize: 12; font.weight: Font.DemiBold }
                        ComboBox { id: scaleBox; Layout.fillWidth: true; implicitHeight: 44; model: ["0.75", "1.00", "1.25", "1.50", "1.75", "2.00"] }
                    }
                    FormField {
                        id: portField
                        label: "VNC-port"
                        validator: IntValidator { bottom: 1024; top: 65535 }
                        inputMethodHints: Qt.ImhDigitsOnly
                    }
                }
                Rectangle { Layout.fillWidth: true; implicitHeight: 64; radius: 10; color: "#101621"; border.color: "#283348"
                    RowLayout { anchors.fill: parent; anchors.margins: 14
                        ColumnLayout {
                            spacing: 2
                            Label { text: "Bruk med Sunshine"; color: "#e7ecf7"; font.weight: Font.DemiBold }
                            Label { text: "Slå av fysiske skjermer under strømming"; color: "#7f8da4"; font.pixelSize: 12 }
                        }
                        Item { Layout.fillWidth: true }
                        Switch { id: sunshineCheck }
                    }
                }
                Label { text: "Output blir: Virtual-" + nameField.text; color: "#708099"; font.pixelSize: 12; font.family: "monospace" }
            }
        }
        footer: RowLayout { height: 78; spacing: 10
            Item { Layout.fillWidth: true }
            SoftButton { visible: editor.profileId.length > 0; text: "Slett"; onClicked: { monitorManager.removeProfile(profileList.currentIndex); editor.close() } }
            SoftButton { text: "Avbryt"; onClicked: editor.close() }
            SoftButton { Layout.rightMargin: 24; text: "Lagre profil"; primary: true; onClicked: monitorManager.saveProfile({id: editor.profileId, name: nameField.text, width: Number(widthField.text), height: Number(heightField.text), refresh: 60, scale: Number(scaleBox.currentText), port: Number(portField.text), sunshine: sunshineCheck.checked, password: editor.password}) }
        }
        Connections { target: monitorManager; function onOperationFinished(success) { if (success) editor.close() } }
        onOpened: nameField.forceActiveFocus()
    }
}
