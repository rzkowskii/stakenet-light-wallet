import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

    Column {
        anchors.centerIn: parent
        Button {
            text: "Get info"
            onClicked: client.getInfo()
        }

        Button {
            text: "Pull invoice"
            onClicked: client.pullInvoice()
        }
        Row {
            TextField {
                id: field
            }

            Button {
                text: "Open channel"
                onClicked: {
                    client.openChannel(field.text, 1);
                }
            }
        }
    }
}
