import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

import Components 1.0

StackViewPage {
    id: welcomePage
    skipButton.visible: false
    backButton.visible: false
    headerText: "STAKENET | DEX"
    headerSource: "qrc:/images/welcome_page_top_logo.svg"
    backButton.onClicked: {
        stackView.pop()
    }

    background: Image {
        anchors.fill: parent
        source: "qrc:/images/ComingSoonPageBg.svg"
    }

    RowLayout {
        anchors.fill: parent
        anchors.topMargin: 67
        anchors.bottomMargin: 83
        spacing: 75

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Rectangle {
            Layout.preferredWidth: 360
            Layout.preferredHeight: 460
            Layout.alignment: Qt.AlignVCenter
            color: "#99151B30"

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                anchors.bottomMargin: 60

                Image {
                    source: "qrc:/images/into_page_ic_bg.svg"
                    sourceSize: Qt.size(196, 222)
                    Layout.alignment: Qt.AlignHCenter

                    Image {
                        source: "qrc:/images/ic_add.svg"
                        anchors.centerIn: parent
                        sourceSize: Qt.size(70, 60)
                    }
                }

                XSNLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Create new wallet"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 19
                    font.bold: true
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Create a new wallet and generate a 24 word seed phrase"
                    Layout.preferredWidth: 220
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 15
                    color: "#7377A5"
                }
            }

            PointingCursorMouseArea {
                onClicked: {
                    stackView.push(getStartedPageComponent)
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 360
            Layout.preferredHeight: 460
            Layout.alignment: Qt.AlignVCenter
            color: "#99151B30"

            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                anchors.bottomMargin: 60

                Image {
                    source: "qrc:/images/into_page_ic_bg.svg"
                    sourceSize: Qt.size(196, 222)
                    Layout.alignment: Qt.AlignHCenter

                    Image {
                        source: "qrc:/images/ic_cloud_restore.svg"
                        sourceSize: Qt.size(70, 49)
                        anchors.centerIn: parent
                    }
                }

                XSNLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Restore from seed"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 19
                    font.bold: true
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 220
                    wrapMode: Text.WordWrap
                    text: "Restore your wallet from a previous 24 word seed phrase"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 15
                    color: "#7377A5"
                }
            }

            PointingCursorMouseArea {
                onClicked: {
                    stackView.push(restoreFromBackupComponent)
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}

