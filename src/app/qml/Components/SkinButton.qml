import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5

import com.xsn.utils 1.0

Button {
    id: skinButton
    property string skinPath: ""

    width: 81
    height: 140

    checkable: true
    checked: SkinColors.isActiveSkin(model.skinPath)

    background: Item {
        id: buttonBackground

        ColumnLayout {

            spacing: 24

            Layout.fillHeight: true
            Layout.fillWidth: true

            Image {
                id: imageBackground
                Layout.preferredHeight: 80
                Layout.preferredWidth: 72

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
                        //                                    SkinColors.initSkin(model.skinPath)
                    }

                    onExited: {
                        buttonBackground.scale = scale
                        //                                    SkinColors.initSkin(group.checkedButton.skinPath);
                    }
                }


                Image {
                    id: selectedIcon
                    source: "qrc:/images/selected_icon.png"
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    visible: skinButton.checked
                }
            }

            XSNLabel {
                id: name
                Layout.alignment: Qt.AlignHCenter
                color: skinButton.checked ? SkinColors.mainText : SkinColors.secondaryText
                text: qsTr(model.skinName)
            }
        }
    }
}

