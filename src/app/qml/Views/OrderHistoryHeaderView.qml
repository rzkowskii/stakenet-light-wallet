import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

RoundedRectangle {
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
        anchors.fill: parent
        anchors.rightMargin: 15
        anchors.leftMargin: 15
        spacing: 0

        OrderBookHeaderText {
            Layout.fillHeight: true
            Layout.preferredWidth: 0.2 * parent.width
            text: "Date"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        OrderBookHeaderText {
            Layout.fillHeight: true
            Layout.preferredWidth: 0.1 * parent.width
            text: "Pair"
            visible: false
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        OrderBookHeaderText {
            Layout.fillHeight: true
            Layout.preferredWidth: 0.1 * parent.width
            text: "Side"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        OrderBookHeaderText {
            Layout.fillHeight: true
            Layout.preferredWidth: 0.3 * parent.width
            text: "Price (%1)".arg(currentQuoteAssetSymbol)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        OrderBookHeaderText {
            Layout.fillHeight: true
            Layout.preferredWidth: 0.2 * parent.width
            text: "Filled (%1)".arg(currentBaseAssetSymbol)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        OrderBookHeaderText {
            Layout.fillHeight: true
            Layout.preferredWidth: 0.2 * parent.width
            text: "Total (%1)".arg(currentQuoteAssetSymbol)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
