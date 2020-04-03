    import QtQuick 2.11
    import QtQuick.Controls 2.4

    import QtQuick.Layouts 1.11
    import Ufo.Wallet 1.0
    import "."

    Dialog {
        id: dialog
        modal: true

        signal saveClicked()
        property var fileName: fileNameInput.text

        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        height: contentItem.implicitHeight
    
        parent: Overlay.overlay
        padding: 0

        closePolicy: Popup.NoAutoClose | Popup.CloseOnEscape

        onClosed: {
            fileNameInput.text = ""
        }

        onOpened: {
            fileNameInput.forceActiveFocus();
        }

        background: Rectangle {
            radius: 10
            color: Style.background_popup
            anchors.fill: parent
        }

        contentItem: ColumnLayout {
            GridLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 400
                Layout.margins: 30
                rowSpacing: 20
                columnSpacing: 13
                columns: 2

                RowLayout {
                    Layout.columnSpan: 2
                    SFText {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        leftPadding: 30
                        font.pixelSize: 18
                        font.styleName: "Bold"
                        font.weight: Font.Bold
                        color: Style.content_main
                        text: "Save transactions"
                    }

                    CustomToolButton {
                        icon.source: "qrc:/assets/icon-cancel.svg"
                        icon.width: 12
                        icon.height: 12
                        //% "Close"
                        ToolTip.text: qsTrId("general-close")
                        onClicked: {
                            dialog.close();
                        }
                    }
                }

                ColumnLayout {
                    id: verifyLayout
                    Layout.fillWidth: true
                    Layout.columnSpan: 2
                    Layout.alignment: Qt.AlignTop
                    visible: shouldVerify
                    SFText {
                        Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                        opacity: 0.5
                        font.pixelSize: 14
                        color: Style.content_main
                        //% "Paste your payment proof here"
                        text: "Enter the name of the file you want to save(*.csv)"
                    }
            

                    SFTextInput {
                            id: fileNameInput
                            Layout.fillWidth: true
                            focus: true
                            activeFocusOnTab: true
                            font.pixelSize: 14
                            color: Style.content_main
                            text: ""
                            validator:        RegExpValidator { regExp: /[0-9a-zA-Z_\-\=\+\(\)\[\]]{1,}/ }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.topMargin: -16
                        color: fileNameInput.color
                        Layout.preferredHeight: (fileNameInput.activeFocus || fileNameInput.hovered) ? 2 : 1
                        opacity: (fileNameInput.activeFocus || fileNameInput.hovered) ? 0.3 : 0.1
                    }
                
                    PrimaryButton {
                        id: saveBtn
                        Layout.preferredHeight: 38
                        Layout.preferredWidth: 125
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 10
                        text: "save"
                        icon.source: "qrc:/assets/icon-done.svg"
                        enabled: fileNameInput.text !== ""
                        onClicked: {
                            dialog.saveClicked()
                            dialog.close()
                        }
                    }
                }

            }

        }
    }