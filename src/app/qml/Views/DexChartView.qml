import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtWebEngine 1.10
import QtQuick.Window 2.2

Item {

    Component {
        id: tradingViewComponent

        Window {
            title: qsTr("TradingView")
            property WebEngineView webView: tradeWebEngineView
            width: 700
            height: 600

            WebEngineView {
                id: tradeWebEngineView
                anchors.fill: parent
            }
        }
    }

    WebEngineView {
        id: webEngineView
        anchors.fill: parent
        url: "http://orderbook.stakenet.io/graph/%1_%2".arg(
                 currentBaseAssetSymbol).arg(currentQuoteAssetSymbol)

        onNewViewRequested: {
            var window = openTradingViewWindow()
            request.openIn(window.webView)
        }
    }

    function openTradingViewWindow(){
        var window = tradingViewComponent.createObject(mainWindow)
        window.show();
        return window;
    }
}
