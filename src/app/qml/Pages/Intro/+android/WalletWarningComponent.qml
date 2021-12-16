import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0

import com.xsn.utils 1.0
import com.xsn.viewmodels 1.0

ColumnLayout {
    Layout.alignment: Qt.AlignCenter

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    ColumnLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: true
        Layout.preferredHeight: 50
        Layout.topMargin: 300
        spacing: 10

        Text {
            Layout.alignment: Qt.AlignHCenter
            color: SkinColors.mainText
            text: "Create wallet without password?"
            font.pixelSize: 18
            font.family: regularFont.name
            font.weight: Font.Medium
        }

        Text {
            color: SkinColors.mainText
            Layout.alignment: Qt.AlignHCenter
            text: "You could lose all your funds."
            font.pixelSize: 18
            font.family: regularFont.name
            font.weight: Font.Medium
        }
    }

    Item {
        Layout.fillHeight: true
    }

    MobileFooter {
        Layout.bottomMargin: 30
        Layout.leftMargin: 25
        Layout.rightMargin: 25
        leftButton.text: "cancel"
        rightButton.text: "create"
        onLeftButtonClicked: {
            stackView.pop();
        }
        onRightButtonClicked: {
            stackView.push(creatingWalletComponent);
        }
    }
}
