import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

RoundedRectangle {
    property bool hasSwapPair
    property alias model: repeater.model
    property string textColor: ""
    corners.topLeftRadius: 10
    corners.topRightRadius: 10
    customGradient: {
        "vertical": true,
        "colors": [{
                       "position": 0.0,
                       "color": SkinColors.delegatesBackgroundLightColor
                   }, {
                       "position": 1.0,
                       "color": SkinColors.delegatesBackgroundDarkColor
                   }]
    }

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.rightMargin: 15
        anchors.leftMargin: 10
        spacing: 0

        Repeater {
            id: repeater

            delegate: OrderBookHeaderText {
                Layout.preferredWidth: parent.width * model.size
                Layout.fillHeight: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 10
                text: hasSwapPair ? "%1 (%2)".arg(model.name).arg(
                                        model.name === "Amount" ? currentBaseAssetSymbol : currentQuoteAssetSymbol) : model.name
                color: SkinColors.headerText
            }
        }
    }
}
