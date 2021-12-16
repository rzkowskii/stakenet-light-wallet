import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

import Components 1.0

ColumnLayout {
    spacing: 65

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    Image {
        Layout.topMargin: 35
        Layout.alignment: Qt.AlignHCenter
        source: "qrc:/images/AW_RESTORE_ERROR.png"
    }

    ColumnLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillHeight: true
        spacing: 20

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Oops!"
            font.family: regularFont.name
            font.pixelSize: 48
            color: SkinColors.mainText
            font.weight: Font.DemiBold
        }

        XSNLabel {
            Layout.alignment: Qt.AlignHCenter
            text: "Something wrong!"
            font.pixelSize: 17
            color: SkinColors.transactionItemSent
        }

        SecondaryLabel {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillHeight: true
            font.pixelSize: 16
            text: "Your 24 word seed phrase didn`t match. \n Please try again"
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Text {
        Layout.preferredHeight: 20
        Layout.alignment: Qt.AlignHCenter
        font.capitalization: Font.AllUppercase
        font.pixelSize: 16
        font.family: regularFont.name
        color: SkinColors.mainText
        text: "Try again"

        PointingCursorMouseArea {
            onClicked: {
                restoreStackView.pop();
                restoreStackView.pop();
                restoreStackView.pop();
            }
        }
    }
}
