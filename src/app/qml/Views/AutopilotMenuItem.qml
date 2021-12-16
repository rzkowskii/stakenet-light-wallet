import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

import "../Components"
import "../Views"
import "../Popups"

MenuItem {
    id: autopilotItem
    property PaymentNodeViewModel payNodeViewModel

    contentItem: Item {
        PointingCursorMouseArea {
            anchors.fill: parent
            onEntered: backGr.startFade()
            onExited: backGr.stopFade()
            onClicked: autopilotItem.triggered()
        }

        RowLayout {
            anchors.leftMargin: 7
            anchors.rightMargin: 7
            anchors.fill: parent
            spacing: 15

            Image {
                source: autopilotItem.highlighted ? "qrc:/images/ln_autopilot_active.png" : "qrc:/images/LN_autopilot_default.png"
                sourceSize: Qt.size(15, 15)
            }

            Row {
                spacing: 5

                FadedText {
                    id: autopilotLabel
                    text: "LN Autopilot"
                    opacity: enabled ? 1.0 : 0.5
                    color: autopilotItem.highlighted ? SkinColors.mainText : SkinColors.secondaryText
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    font.pixelSize: 14
                    font.family: regularFont.name
                    font.capitalization: Font.MixedCase

                    MouseArea {
                        id: lnAutopilotMouseArea
                        hoverEnabled: true
                        anchors.fill: parent
                        onEntered: autopilotLabel.startFade();
                        onExited: autopilotLabel.stopFade();
                    }
                }

                SwitchComponent {
                    id: switchComponent
                    Layout.alignment: Qt.AlignRight
                    checked: payNodeViewModel.type === Enums.PaymentNodeType.Lnd && payNodeViewModel.stateModel.autopilotActive
                    onClicked: {
                        payNodeViewModel.activateAutopilot(!checked)
                    }
                }

                CustomToolTip {
                    id: customToolTip
                    height: 41
                    width: 230
                    x: -55
                    parent: autopilotLabel
                    visible: lnAutopilotMouseArea.containsMouse
                    tailPosition: width / 2
                    text: "Auto deposit coins into lightning"
                }
            }
        }
    }

    background: FadedRectangle {
        id: backGr
        anchors.fill: parent
        anchors.margins: 1
        opacity: enabled ? 1 : 0.5
        activeStateColor: SkinColors.headerBackground
        inactiveStateColor: SkinColors.menuBackground
    }
}
