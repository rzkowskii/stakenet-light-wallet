import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

StackViewPage {
    skipButton.visible: false
    headerText: "STAKENET | DEX"
    headerSource: SkinColors.mainWalletLogo
    backButton.onClicked: {
        stackView.pop()
    }

    background: Image {
        anchors.fill: parent
        source: "qrc:/images/ComingSoonPageBg.svg"
    }

    property string mnemonicPhrase
    property var mnemonicList
    property var randomMnemonic: []
    property var shuffleMnemonic: []

    ListModel {
        id: userConfirmedMnemonicModel
    }

    Component.onCompleted: {

        mnemonicList = mnemonicPhrase.split(' ')

        while (userConfirmedMnemonicModel.count !== 12) {

            var index = randomInteger(0, 23)

            if (!randomMnemonic.includes(mnemonicList[index])) {

                userConfirmedMnemonicModel.append({
                                                      "mnemonicIndex": index,
                                                      "word": ""
                                                  })
                randomMnemonic.push(mnemonicList[index])
            }
        }

        shuffleMnemonic = randomMnemonic
        shuffleArray(shuffleMnemonic)

        gridRandomMnemonic.model = shuffleMnemonic
    }

    function randomInteger(min, max) {
        return Math.floor(Math.random() * (max - min + 1)) + min
    }

    function shuffleArray(array) {
        for (var i = array.length - 1; i > 0; i--) {
            var j = Math.floor(Math.random() * (i + 1))
            var temp = array[i]
            array[i] = array[j]
            array[j] = temp
        }
    }

    function ifEmptyItemExists() {
        var emptyItemExists = false

        for (var i = 0; i < userConfirmedMnemonicModel.count; i++) {
            if (userConfirmedMnemonicModel.get(i).word === "") {
                emptyItemExists = true
                return emptyItemExists
            }
        }

        return emptyItemExists
    }

    function checkIfSeedIsCorrect() {
        var seedIsCorrect = true

        for (var i = 0; i < userConfirmedMnemonicModel.count; i++) {
            if (userConfirmedMnemonicModel.get(
                        i).word !== mnemonicList[userConfirmedMnemonicModel.get(
                                                     i).mnemonicIndex]) {
                seedIsCorrect = false
                return seedIsCorrect
            }
        }

        return seedIsCorrect
    }

    RowLayout {
        anchors.fill: parent
        anchors.topMargin: 30

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Rectangle {
            Layout.preferredWidth: 960
            Layout.preferredHeight: 559
            Layout.alignment: Qt.AlignVCenter
            color: "#99151B30"

            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                Image {
                    source: "qrc:/images/ic_shield24.svg"
                    sourceSize: Qt.size(170, 193)
                    Layout.alignment: Qt.AlignHCenter
                }

                XSNLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Confirm your seed phrase"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 19
                    font.bold: true
                }

                Text {
                    Layout.preferredWidth: 700
                    Layout.alignment: Qt.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: "Please test your recollection of your seed phrase by entering the corresponding words in the boxes below. The number in each box corresponds to the word position of your 24-word seed phrase (i.e. \"10\" refers to the 10th word in your seed phrase)."
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 15
                    color: "#7377A5"
                }

                GridView {
                    id: checkGrid
                    Layout.preferredHeight: 80
                    Layout.preferredWidth: 720
                    Layout.leftMargin: 30
                    Layout.rightMargin: 20
                    Layout.alignment: Qt.AlignHCenter
                    cellWidth: 118
                    cellHeight: 40

                    model: userConfirmedMnemonicModel

                    highlightFollowsCurrentItem: true
                    highlight: Rectangle {
                        color: "transparent"

                        Rectangle {
                            anchors.fill: parent
                            color: "transparent"
                            anchors.margins: 4
                            border.color: "#DEE0FF"
                            radius: 15
                            border.width: 1
                        }
                    }

                    delegate: Item {
                        id: delegate
                        width: checkGrid.cellWidth
                        height: checkGrid.cellHeight

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 5

                            color: "#1FDEE0FF"
                            radius: 20

                            Row {
                                anchors.fill: parent
                                spacing: 10

                                Rectangle {
                                    width: 30
                                    height: 30
                                    radius: width * 0.5
                                    color: "#807377A5"

                                    XSNLabel {
                                        id: labelIndex
                                        font.pixelSize: 16
                                        color: SkinColors.mainText
                                        anchors.centerIn: parent
                                        font.family: regularFont.name
                                        font.bold: true
                                        text: mnemonicIndex + 1
                                    }
                                }

                                XSNLabel {
                                    id: label
                                    font.pixelSize: 14
                                    color: SkinColors.menuItemText
                                    anchors.verticalCenter: parent.verticalCenter
                                    horizontalAlignment: Text.AlignRight
                                    font.family: mediumFont.name
                                    text: word
                                }
                            }

                            PointingCursorMouseArea {
                                id: mouseArea
                                onClicked: {
                                    checkGrid.currentIndex = index
                                }
                            }
                        }
                    }
                }

                GridView {
                    id: gridRandomMnemonic
                    Layout.preferredWidth: 500
                    Layout.preferredHeight: 80
                    Layout.alignment: Qt.AlignHCenter
                    Layout.leftMargin: 50
                    Layout.rightMargin: 20
                    cellWidth: 72
                    cellHeight: 40

                    model: shuffleMnemonic

                    highlightFollowsCurrentItem: true
                    highlight: Rectangle {
                        color: "transparent"

                        Rectangle {
                            anchors.fill: parent
                            color: "transparent"
                            anchors.margins: 4
                            border.color: "#DEE0FF"
                            radius: 15
                            border.width: 1
                        }
                    }

                    delegate: Item {
                        id: delegateRandomMnemonic
                        width: gridRandomMnemonic.cellWidth
                        height: gridRandomMnemonic.cellHeight

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 5

                            color: "#1FDEE0FF"
                            radius: 20

                            XSNLabel {
                                id: randomMnemonicLabel
                                anchors.fill: parent
                                font.pixelSize: 13
                                color: SkinColors.menuItemText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: mediumFont.name
                                text: modelData
                            }

                            PointingCursorMouseArea {
                                id: mouseAreaRandomMnemonic
                                onClicked: {
                                    gridRandomMnemonic.currentIndex = index

                                    userConfirmedMnemonicModel.setProperty(
                                                checkGrid.currentIndex, "word",
                                                modelData)

                                    checkGrid.currentIndex++;
                                }
                            }
                        }
                    }
                }

                IntroButton {
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 35
                    Layout.bottomMargin: 30
                    Layout.topMargin: 20
                    Layout.alignment: Qt.AlignHCenter
                    radius: 8
                    text: "Next"
                    textColor: "white"
                    borderColor: "transparent"
                    buttonColor: "#1254DD"
                    buttonGradientColor: "#1D96EC"
                    buttonHoveredColor: "#1254DD"
                    buttonGradientHoveredColor: "#1D96EC"
                    enabled: !ifEmptyItemExists()

                    onClicked: {

                        if (checkIfSeedIsCorrect()) {
                            if (ApplicationViewModel.walletViewModel.verifyMnemonic(
                                        mnemonicPhrase)) {

                                stackView.push(walletCreatedComponent, {
                                                   "isOpenRescanNotification": true,
                                                   "mainHeaderLbl": "Wallet Created",
                                                   "secondaryHeaderText": "Your wallet has been successfully created. Thank you for using Stakenet DEX!"
                                               })
                            } else {
                                messageDialog.text = "Backup failed!"
                                messageDialog.open()
                            }
                        } else {
                            messageDialog.text = "Wrong mnemonic phrase!"
                            messageDialog.open()
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
