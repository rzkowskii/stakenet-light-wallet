import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Components"
import "../Views"

import com.xsn.utils 1.0

Page {
    id: root

    background: Rectangle {
        color: "transparent"
    }

    FontLoader { id: boldFont; source: "qrc:/Rubik-Bold.ttf" }
    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    StackView {
        id: stackView
        anchors.fill: parent
        anchors.topMargin: 25
        anchors.bottomMargin: 30
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        clip: true
        initialItem: initialComponent

        Component {
            id: initialComponent

            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                spacing: 50

                MobileTitle {
                    Layout.alignment: Qt.AlignCenter
                    text: "help center"
                }

                ColumnLayout {
                    spacing: 40

                    Text {
                        Layout.leftMargin: 26
                        font.family: boldFont.name
                        text: "Suggested for you"
                        font.pixelSize: 24
                        color: SkinColors.mainText
                        font.bold: true
                    }

                    ListView {
                        id: listView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        boundsBehavior: Flickable.StopAtBounds
                        spacing: 0

                        model: ListModel {
                            ListElement { title: "Create new wallet"; component: ""}
                            ListElement { title: "Deposit"; component: ""}
                            ListElement { title: "Withdraw"; component: ""}
                            ListElement { title: "Lightning swap"; component: ""}
                            ListElement { title: "Placeholder for next questions?"; component: ""}
                            ListElement { title: "Placeholder for next questions?"; component: ""}
                            ListElement { title: "Placeholder for next questions?"; component: ""}
                            ListElement { title: "Placeholder for next questions?"; component: ""}
                        }

                        delegate: Item {
                            height: 51
                            width: parent.width

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 0

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.leftMargin: 11
                                    spacing: 20

                                    Image {
                                        Layout.alignment: Qt.AlignHCenter
                                        source: "qrc:/images/IC_QA.png"
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        font.pixelSize: 14
                                        font.family: regularFont.name
                                        text: model.title
                                        color: SkinColors.mainText
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 1
                                    color: SkinColors.mobileLineSeparator
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                color: /*mouseArea.contains ? SkinColors.highlightedMenuItem :*/ "transparent"
                            }

                            PointingCursorMouseArea {
                                id: mouseArea
                                onClicked: stackView.push(createNewWallet)
                            }
                        }

                        MobileButton {
                            checkable: true
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 5
                            width: 56
                            height: 36
                            image.imageSource: "qrc:/images/magnifyingGlass.png"
                            image.color: SkinColors.magnifyingGlass
                            image.imageSize: 30
                            backgroundButton.border.color: SkinColors.menuBackgroundGradientFirst
                            backgroundButton.color:  SkinColors.mobileButtonBackground
                            backgroundButton.radius: 18
                            z: 1000
                        }
                    }
                }

                MobileFooter {
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    leftButton.text: "back"
                    rightButton.text: "ask us"
                    onLeftButtonClicked: {
                        navigateBack()
                    }
                }
            }
        }

        Component {
            id: createNewWallet

            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                spacing: 50

                MobileTitle {
                    Layout.alignment: Qt.AlignCenter
                    text: "help center"
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 40

                    Text {
                        Layout.leftMargin: 26
                        font.family: boldFont.name
                        text: "Create a new wallet"
                        font.pixelSize: 24
                        color: SkinColors.mainText
                        font.bold: true
                    }

                    ListView {
                        id: listView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        boundsBehavior: Flickable.StopAtBounds
                        spacing: 0

                        model: ListModel {
                            ListElement { name: "Step 1"; explanation: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque sed malesuada mi. Sed blandit porta bibendum. Etiam suscipit faucibus lorem ac vulputate. Etiam rhoncus, est non condimentum condimentum, nulla turpis sollicitudin dolor, molestie dictum ante purus nec lectus.Maecenas ac sodales felis. Nunc facilisis ligula posuere condimentum viverra. Cras ultrices tellus sed elementum mattis. "}
                            ListElement { name: "Step 2"; explanation: "Nullam vel vestibulum erat. Aliquam sem quam, dictum in tortor eget, maximus vestibulum erat. Pellentesque imperdiet lobortis tellus, in porttitor neque gravida a. Fusce et viverra enim, ut sollicitudin risus. Donec condimentum orci sed quam luctus auctor.Maecenas iaculis cursus velit nec fringilla. Fusce ullamcorper maximus pellentesque. Duis id massa rutrum, luctus tortor vel, vulputate nulla."}
                            ListElement { name: "Step 3"; explanation: "Donec et finibus velit. Nullam eleifend sapien scelerisque velit fermentum rhoncus vitae id orci. Praesent laoreet nisl neque, eu tristique purus scelerisque sed. Ut condimentum ex sagittis elit auctor, in malesuada quam euismod. Pellentesque ac arcu tempor, ultricies mi nec, imperdiet leo. "}
                            ListElement { name: "Step 4"; explanation: "Uis vitae pretium ante. Sed ultricies quis orci non varius. Curabitur rutrum in risus egestas ultrices. Etiam nec sem finibus, tristique lacus quis, convallis est. Duis nunc dolor, sodales ut dui id, commodo tristique diam."}
                        }

                        delegate: ColumnLayout {
                            id: delegateItem
                            width: parent.width
                            spacing: 0

                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: delegateItem.ListView.isCurrentItem ? explanation.contentHeight + 70 : 50

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.topMargin: delegateItem.ListView.isCurrentItem ? 15 : 0
                                    anchors.bottomMargin: delegateItem.ListView.isCurrentItem ? 30 : 0
                                    spacing: 28

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 50
                                        Layout.leftMargin: 10
                                        Layout.alignment: Qt.AlignVCenter
                                        spacing: 40

                                        Image {
                                            Layout.alignment: Qt.AlignVCenter
                                            source: delegateItem.ListView.isCurrentItem ? "qrc:/images/ic_min.png" : "qrc:/images/ic_plus.png"
                                        }

                                        Text {
                                            font.pixelSize: 14
                                            font.family: regularFont.name
                                            text: model.name
                                            color: SkinColors.mainText
                                        }
                                    }

                                    SecondaryLabel {
                                        id: explanation
                                        visible: delegateItem.ListView.isCurrentItem
                                        Layout.alignment: Qt.AlignVCenter
                                        Layout.leftMargin: 10
                                        Layout.fillWidth: true
                                        text: model.explanation
                                        font.family: regularFont.name
                                        wrapMode: Text.WordWrap
                                    }
                                }

                                PointingCursorMouseArea {
                                    onClicked: listView.currentIndex = index
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: SkinColors.mobileLineSeparator
                            }
                        }
                    }
                }

                MobileFooter {
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    leftButton.text: "back"
                    rightButton.text: "ask us"
                    onLeftButtonClicked: {
                        stackView.pop()
                    }
                }
            }
        }
    }
}
