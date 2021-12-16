import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

Rectangle {
    id: root
    anchors.fill: parent
    color: "#090D1C"
    signal addTransaction(int count)
    signal generateBlocks(int count, string addressTo)
    signal clearTransactions()
    signal reorgChain(int disconnectBlocks, int connectBlocks, string addressTo)
    property int maxHeight: 0

    ColumnLayout {
        spacing: 20
        width: parent.width
        height: parent.height

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        Button {
            text: "Add transaction"
            Layout.fillWidth: true
            onClicked: addTransaction(1);
        }
        Button {
            text: "Add 10 transactions"
            Layout.fillWidth: true
            onClicked: addTransaction(10);
        }
        Button {
            text: "Add 50 transactions"
            Layout.fillWidth: true
            onClicked: addTransaction(50);
        }
        Button {
            text: "Add 100 transactions"
            Layout.fillWidth: true
            onClicked: addTransaction(100);
        }
        Button {
            text: "Clear transactions"
            Layout.fillWidth: true
            onClicked: clearTransactions();
        }

        Label {
            text: "Max chain height: %1".arg(maxHeight)
            color: "white"
        }

        Button {
            text: "Generate"
            onClicked: generateBlocks(numberOfBlocks.value, recevingAddress.text)
        }

        Button {
            text: "Reorganize"
            onClicked: reorgChain(disconnectBlocks.value, numberOfBlocks.value, recevingAddress.text)
        }

        Label {
            text: "Connect blocks"
            color: "white"
        }

        SpinBox {
            id: numberOfBlocks
            editable: true
            to: 1000
            value: 1
        }

        Label {
            text: "Disconnect blocks"
            color: "white"
        }

        SpinBox {
            id: disconnectBlocks
            editable: true
            to: 1000
            value: 0
        }

        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: "white"

            TextInput {
                id: recevingAddress
                anchors.fill: parent
                anchors.margins: 5
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}
