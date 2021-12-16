import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5

import com.xsn.utils 1.0
import "../"

Button {
    id: skinButton
    property string skinPath: ""
    width: 105
    height: 150

    checkable: true
    checked: SkinColors.isActiveSkin(model.skinPath)

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    background: Item {
        id: buttonBackground

        ColumnLayout {
            anchors.fill: parent
            spacing: 5
            Layout.alignment: Qt.AlignCenter
            Layout.fillHeight: true
            Layout.fillWidth: true

            Image {
                id: imageBackground
                Layout.alignment: Qt.AlignCenter
                Layout.preferredHeight: 100
                Layout.preferredWidth: 89

                source: model.skinIcon

                MouseArea {
                    anchors.fill: parent
                    cursorShape:  Qt.PointingHandCursor
                    hoverEnabled: true

                    onClicked: {
                        SkinColors.initSkin(model.skinPath, model.skinName)
                        skinButton.checked = true
                    }

                    onEntered:  {
                        buttonBackground.scale = scale * 1.05
                    }

                    onExited: {
                        buttonBackground.scale = scale
                    }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredHeight:  4
                Layout.preferredWidth: 20
                radius: 1
                opacity: skinButton.checked ? 1 : 0.4
                color: SkinColors.menuBackgroundGradientFirst
            }

            XSNLabel {
                id: name
                font.pixelSize: 12
                font.family: regularFont.name
                Layout.alignment: Qt.AlignHCenter
                color: skinButton.checked ? SkinColors.mainText : SkinColors.secondaryText
                text: qsTr(model.skinName)
            }
        }
    }
}
