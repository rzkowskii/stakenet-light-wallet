import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Page {
    id: root
    property int loadingProcess
    property bool appLoaded: false

    property WalletViewModel viewModel: ApplicationViewModel.walletViewModel

    Component.onCompleted: {
        if (!appLoaded) {
            // we need to do this to prevent next situation:
            // loadApp emits onLoadedChanged and we immideatelly
            // handle that slot in QML by replacing current view, which is still in
            // onCompleted handler. MSVC crashes on this.
            // We solve it by executing this call on event loop instead of direct call.
            Qt.callLater(function() {
                viewModel.loadApp()
            })
        }
    }

    FontLoader {
        id: mediumFont
        source: "qrc:/Rubik-Medium.ttf"
    }

    Connections {
        target: viewModel

        function onLoadingProcessChanged(process) {
            console.log("Loading ", process)
            loadingProcess = process
        }

        function onLoadingProcessFailed(process) {
            stackView.push(errorComponent, {
                               "loadingProcessError": process
                           })
        }

        function onLoadedChanged(isLoaded) {
            if (isLoaded) {
                replaceView(mainPage)
            } else {
                replaceView(introScreen)
            }
        }

        function onRequestMnemonicFinished() {
            ApplicationViewModel.walletViewModel.restoreWallet(mnemonic)
        }

        function onRestoredChanged() {
            replaceView(mainPage)
        }
    }

    StackView {
        id: stackView
        anchors.centerIn: parent
        anchors.fill: parent
        anchors.margins: 20
        initialItem: loadingWalletComponent
    }

    Component {
        id: loadingWalletComponent
        LoadingWalletComponent {}
    }

    Component {
        id: loadingComponent

        LoadingComponent {
            process: loadingStringFromState(loadingProcess)
        }
    }

    Component {
        id: errorComponent

        ColumnLayout {
            property int loadingProcessError
            spacing: 50

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                Layout.alignment: Qt.AlignHCenter

                XSNLabel {
                    anchors.centerIn: parent
                    text: "Loading %1 failed!".arg(loadingStringFromState(
                                                       loadingProcessError))
                    font.family: mediumFont.name
                    font.pixelSize: 20
                }
            }

            IntroButton {
                Layout.preferredWidth: 150
                Layout.preferredHeight: 45
                Layout.alignment: Qt.AlignHCenter

                text: {
                    var type = ""
                    switch (loadingProcessError) {
                    case WalletViewModel.LoadingState.Chain:
                        return "Rescan chain"
                    case WalletViewModel.LoadingState.Transactions:
                        return "Rescan txns"
                    case WalletViewModel.LoadingState.Wallet:
                        return "Reset wallet"
                    default:
                        break
                    }

                    return ""
                }

                onClicked: {
                    viewModel.resetState(loadingProcessError)
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }

    background: Rectangle {
        anchors.fill: parent
        color: SkinColors.walletPageHeaderViewBlueColor
    }

    function loadingStringFromState(state) {
        switch (state) {
        case WalletViewModel.LoadingState.Chain:
            return "chain"
        case WalletViewModel.LoadingState.Transactions:
            return "transactions"
        case WalletViewModel.LoadingState.Wallet:
            return "wallet"
        default:
            break
        }

        return ""
    }
}
