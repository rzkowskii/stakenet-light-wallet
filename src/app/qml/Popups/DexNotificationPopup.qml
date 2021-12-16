import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root
    width: 600
    height: 350

    property WalletDexViewModel walletDexViewModel
    property string baseSymbol: ""
    property string quoteSymbol: ""

    property int baseAssetID: -1
    property int quoteAssetID: -1

    Connections {
        target: walletDexViewModel
        function onSwapAssetsChanged() {
            timer.start();
        }
    }

    Timer {
        id: timer
        interval: 1000;
        onTriggered: close()
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
                text: !walletDexViewModel.hasSwapPair ? "Setting %1/%2 will enable lightning on those coins and disable others. \nAre you sure you want to set %1/%2 for swap?" .arg(baseSymbol) .arg(quoteSymbol)
                                                      : "At the moment changing trading pairs will clear any orders\n placed under the current pair. \nAre you sure you want to proceed?"
                wrapMode: Text.WordWrap
                font.family: regularFont.name
                font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter
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
                    onClicked: reject();
                }

                IntroButton {
                    id: confirmButton
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 150
                    font.pixelSize: 12
                    text: "Confirm"
                    onClicked: {
                        ApplicationViewModel.paymentNodeViewModel.changeSwapAssets(baseAssetID, quoteAssetID, "%1_%2".arg(baseSymbol).arg(quoteSymbol))
                        stackView.push("qrc:/Components/PopupProgressBarComponent.qml", {progressBarMsg: "Changing swap pair ..."})
                    }
                }
            }
        }
    }
}
