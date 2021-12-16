import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"
import "../Views"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Page {
    id: root

    background: Rectangle {
        color: "transparent"
    }

    Component {
        id: mnemomicMobilePageComponent
        MnemonicMobilePage {
        }
    }

    Component {
       id: mobileSkinsComponent
       MobileSkinsPage {

       }
    }

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 30
        anchors.bottomMargin: 20
        spacing: 25

        MobileTitle {
            Layout.alignment: Qt.AlignHCenter
            text: "Personalize"
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 15

            SecondaryLabel {
                text: "notifications"
                font.capitalization: Font.AllUppercase
                Layout.leftMargin: 5
                font.pixelSize: 12
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 49
                radius: 8
                color: SkinColors.mobileSecondaryBackground

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15

                    XSNLabel {
                        Layout.alignment: Qt.AlignVCenter
                        color: SkinColors.mainText
                        text: "App notifications"
                        font.pixelSize: 14
                    }

                    SwitchComponent {
                        Layout.alignment: Qt.AlignRight
                        enabled: false
                        checked: false
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 15

            SecondaryLabel {
                text: "language"
                font.capitalization: Font.AllUppercase
                Layout.leftMargin: 5
                font.pixelSize: 12
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 49
                radius: 8
                color: SkinColors.mobileSecondaryBackground

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15

                    XSNLabel {
                        Layout.alignment: Qt.AlignVCenter
                        color: SkinColors.mainText
                        text: "English - US"
                        font.pixelSize: 14
                    }

                    Image {
                        Layout.alignment: Qt.AlignRight
                        source: "qrc:/images/ic_rightarrow1x.svg"
                        rotation: 90
                        visible: false
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 15

            SecondaryLabel {
                text: "security"
                font.capitalization: Font.AllUppercase
                Layout.leftMargin: 5
                font.pixelSize: 12
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 49
                radius: 8
                color: SkinColors.mobileSecondaryBackground

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15

                    XSNLabel {
                        Layout.alignment: Qt.AlignVCenter
                        color: SkinColors.mainText
                        text: "Required password to open the app"
                        font.pixelSize: 14
                    }

                    SwitchComponent {
                        Layout.alignment: Qt.AlignRight
                        enabled: false
                        checked: false
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 49
                radius: 8
                color: SkinColors.mobileSecondaryBackground

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15

                    XSNLabel {
                        Layout.alignment: Qt.AlignVCenter
                        color: SkinColors.mainText
                        text: "24 words seed"
                        font.pixelSize: 14
                    }

                    Image {
                        Layout.alignment: Qt.AlignRight
                        source: "qrc:/images/ic_rightarrow1x.svg"
                    }
                }

                PointingCursorMouseArea {
                    onClicked: navigateToItem(mnemomicMobilePageComponent);
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 15

            SecondaryLabel {
                text: "Skins"
                font.capitalization: Font.AllUppercase
                Layout.leftMargin: 5
                font.pixelSize: 12
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 49
                radius: 8
                color: SkinColors.mobileSecondaryBackground

                XSNLabel {
                    anchors.leftMargin: 15
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    color: SkinColors.mainText
                    text: SkinColors.skinName
                    font.pixelSize: 14
                }

                PointingCursorMouseArea {
                    onClicked: navigateToItem(mobileSkinsComponent);
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        MobileFooter {
            Layout.leftMargin: 13
            Layout.preferredHeight: 40
            leftButton.text: "back"
            onLeftButtonClicked: {
                navigateBack()
            }
        }
    }
}
