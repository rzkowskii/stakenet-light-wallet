import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root

    popUpText: "Activate Lightning"
    width: 650
    height: 400

    property var lightningsList
    property WalletAssetsListModel assetModel
    property int disableAsset: -1
    property int freeSlots: 0
    property int assetID
    property string assetName: ""

    Component.onCompleted: ApplicationViewModel.paymentNodeViewModel.requestFreeSlots();

    Connections {
        target: ApplicationViewModel.paymentNodeViewModel
        function onFreeSlotsFinished(freeSlots) {
            root.freeSlots = freeSlots;
        }

        function onNodeActivityChanged() {
            timer.start();
        }

        function onLndActivityChangeFailed(errorMessage) {
            stackView.push(changingFailed, { message : errorMessage});
        }
    }

    Timer {
        id: timer
        interval: 1000;
        onTriggered: {
            close();
        }
    }

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    StackView {
        id: stackView
        anchors.fill: parent
        anchors.margins: 30
        clip: true

        initialItem: ColumnLayout {

            spacing: 20

            Text {
                Layout.fillWidth: true
                Layout.preferredHeight: 25
                lineHeight: 1.5
                color: SkinColors.mainText
                text: "Activating the %1 L2 client will deactivate %2.\nPlease confirm to continue.\nIf you have orders, they will be deleted!".arg(assetName).arg(disableAsset === -1 ?"selected coin" : assetModel.get(assetModel.getInitial(disableAsset)).name)
                wrapMode: Text.WordWrap
                font.family: regularFont.name
                font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 20

                Repeater {
                    id: repeater
                    model: lightningsList.map(function(e){return e.toString()})

                    delegate: Button {
                        id: button

                        width: 90
                        height: 90
                        checkable: true
                        checked: lightningsList.length === 1 || disableAsset === id

                        property var current: assetModel.getInitial(Number(modelData))
                        property var name: assetModel.get(current).name
                        property var id: assetModel.get(current).id

                        Component.onCompleted: {
                            if(lightningsList.length === 1)
                            {
                                disableAsset = id
                            }
                        }

                        background: Item {
                            implicitWidth: 80
                            implicitHeight: 80
                        }

                        contentItem: Item {

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 15

                                Image {
                                    Layout.alignment: Qt.AlignHCenter
                                    source: "qrc:/images/ICON_%1.svg".arg(name)
                                    sourceSize: Qt.size(70, 80)

                                    PointingCursorMouseArea {
                                        onEntered:  {
                                            button.scale = scale * 1.05
                                        }

                                        onExited: {
                                            button.scale = scale
                                        }

                                        onClicked: {
                                            if(!button.checked)
                                            {
                                                disableAsset = id
                                            }
                                            else
                                            {
                                                if(lightningsList.length !== 1){
                                                    disableAsset = -1
                                                }
                                            }
                                        }
                                    }

                                    Image {
                                        id: selectedIcon
                                        source: "qrc:/images/selected_icon.png"
                                        anchors.bottom: parent.bottom
                                        anchors.right: parent.right
                                        visible: button.checked
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignHCenter
                                    text: name
                                    color: SkinColors.mainText
                                    font.family: regularFont.name
                                    horizontalAlignment: Text.AlignHCenter
                                    font.pixelSize: 14
                                }
                            }
                        }
                    }
                }
            }


            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 25

                IntroButton {
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 150
                    font.pixelSize: 12
                    text: "Cancel"
                    onClicked: close()
                }

                IntroButton {
                    id: activateButton
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 150
                    font.pixelSize: 12
                    text: "Deactivate"
                    enabled: lightningsList.length === 1 || disableAsset !== -1
                    onClicked: {
                        ApplicationViewModel.paymentNodeViewModel.changeNodeActivity([disableAsset], [assetID]);
                        stackView.push("qrc:/Components/PopupProgressBarComponent.qml", {progressBarMsg: "Switching Lightning Assets ..."})
                    }
                }
            }
        }

        Component {
            id: changingFailed

            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 30

                property string message: ""

                Column {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: 60
                    spacing: 10

                    XSNLabel {
                        text: "Sorry, change lightnings activity failed!"
                        font.family: regularFont.name
                        font.pixelSize: 20
                        horizontalAlignment: Text.AlignHCenter
                    }

                    XSNLabel {
                        text: message
                        font.family: regularFont.name
                        font.pixelSize: 20
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                SecondaryButton {
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 150
                    Layout.rightMargin: 20
                    Layout.alignment: Qt.AlignHCenter
                    font.capitalization: Font.AllUppercase
                    font.pixelSize: 12
                    text: "Go back"
                    borderColor: SkinColors.transactionItemSent
                    onClicked: close();
                }
            }
        }
    }
}
