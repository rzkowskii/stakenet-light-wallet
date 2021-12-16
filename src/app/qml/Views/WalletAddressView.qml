import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import "../Components"
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Item {
    id: root
    signal clicked(var type)

    Rectangle {
        anchors.fill: parent
        color: SkinColors.secondaryBackground
    }

    Item {
        width: isMobile ? 350 : 400
        height: 300
        anchors.centerIn: parent

        FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }
        FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: 22
            anchors.bottomMargin: 30
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 20

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                Layout.alignment: Qt.AlignHCenter

                XSNLabel {
                    font.family: mediumFont.name
                    font.pixelSize: 24
                    text: "Address type"
                    color: SkinColors.mainText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 20
                Layout.leftMargin: 10

                XSNLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width
                    text: "Set your preferred address type."
                    font.pixelSize: 15
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    horizontalAlignment: isMobile ? Text.AlignHCenter : Text.AlignLeft
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            Row {
                Layout.alignment: Qt.AlignHCenter
                spacing: isMobile ? 20 : 50

                Repeater {
                    model: ListModel {
                        ListElement { name: "Legacy"; type: Enums.AddressType.P2PKH; }
                        ListElement { name: "Native-Segit"; type: Enums.AddressType.P2WPKH; }
                    }

                    delegate: IntroButton {
                        width: 150
                        height: 40
                        text: model.name
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: root.clicked(model.type)
                    }
                }
            }

            MobileFooter {
                visible: isMobile
                Layout.alignment: Qt.AlignBottom
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                leftButton.text: "back"
                rightButton.visible: false
                onLeftButtonClicked: {
                    navigateBack()
                }
            }
        }
    }

}
