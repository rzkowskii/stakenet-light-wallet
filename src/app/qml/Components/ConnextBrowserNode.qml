import QtQuick 2.0
import QtWebEngine 1.8
import QtWebChannel 1.0

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

// This fil implements connext browser node and all communication with C++ code.
// Browser node itself is written in typescript then compiled to raw javascript and included
// as qt resource.
// Browser node is running in web engine and all communication happens with web channels.
Item {
    WebEngineView {
        id: connextView
        visible: false

        onJavaScriptConsoleMessage: {
            console.log("connext:" + message)
        }
        webChannel: WebChannel {
            registeredObjects: [connextBrowserNodeAPI]
        }
        onLoadingChanged: {
            console.log("loading changed:", loadRequest.status)
            browserNodeProxy.processManager.onBrowserNodeReady(
                        loadRequest.status === WebEngineView.LoadSucceededStatus)
        }
    }

    // ViewModel to get easy access to connext process manager
    // to manage start/stop and running state.
    ConnextBrowserNodeProxyViewModel {
        id: browserNodeProxy

        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    // Signal handler for handling start/stop requests from C++.
    Connections {
        target: browserNodeProxy.processManager
        function onRequestStart() {
            if (connextView.url.length === undefined) {
                connextView.url = "qrc:/connext/web/index.html"
            }
        }
        function onRequestStop() {
            console.log("request stop:", connextView.loading)
            connextView.stop()
            connextView.url = ""
            browserNodeProxy.processManager.onBrowserNodeReady(false)
        }
    }

    // Signal handler for handling requests from C++
    Connections {
        target: connextBrowserNodeAPI.transport
        function onInitialize(seq, payload) {
            console.log("Requested to run intialize")
            connextBrowserNodeAPI.initialize(seq, payload)
        }
        function onGetConfig(seq, dummy) {
            connextBrowserNodeAPI.getConfig(seq)
        }
        function onGetStateChannels(seq, dummy) {
            connextBrowserNodeAPI.getStateChannels(seq)
        }
        function onSetup(seq, payload) {
            connextBrowserNodeAPI.setup(seq, payload)
        }
        function onWithdrawDeposit(seq, payload) {
            console.log("Requested to withdraw channel")
            connextBrowserNodeAPI.withdrawDeposit(seq, payload)
        }
        function onGetStateChannel(seq, payload) {
            connextBrowserNodeAPI.getStateChannel(seq, payload)
        }
        function onReconcileDeposit(seq, payload) {
            connextBrowserNodeAPI.reconcileDeposit(seq, payload)
        }
        function onRestoreState(seq, payload) {
            connextBrowserNodeAPI.restoreState(seq, payload)
        }
        function onResolveTransfer(seq, payload) {
            connextBrowserNodeAPI.resolveTransfer(seq, payload)
        }
        function onConditionalTransfer(seq, payload) {
            connextBrowserNodeAPI.conditionalTransfer(seq, payload)
        }
        function onGetTransfers(seq, payload) {
            connextBrowserNodeAPI.getTransfers(seq, payload)
        }
    }

    // This class is a web channel effectivelly which is used by JS code to communicate
    // with QML and then C++. We can't create attached property for WebChannel.id from C++ that's why
    // we are using connextBrowserNodeAPI(QML) <-> ConnextBrowserNodeApiTransport <-> business logic.
    QtObject {
        id: connextBrowserNodeAPI
        WebChannel.id: "connextChannel"
        signal initialize(int seq, var request)
        signal setup(int seq, var channelInfo)
        signal getConfig(int seq)
        signal getStateChannels(int seq)
        signal getStateChannel(int seq, var request)
        signal conditionalTransfer(int seq, var request)
        signal resolveTransfer(int seq, var request)
        signal reconcileDeposit(int seq, var request)
        signal withdrawDeposit(int seq, var request)
        signal sendDepositTx(int seq, var request)
        signal restoreState(int seq, var request)
        signal getTransfers(int seq, var request)

        property ConnextBrowserNodeApiTransport transport: browserNodeProxy.transport

        function dispatch(seq, payload) {
            payload = payload || {}
//            console.log("Requested to dispatch:", seq, payload)
            connextBrowserNodeAPI.transport.dispatch(seq, payload)
        }
        function dispatchErr(seq, error) {
            console.log("Requested to dispatch error:", seq, error)
            connextBrowserNodeAPI.transport.dispatchError(seq, error)
        }
        function eventConditionalTransferCreated(payload) {
            connextBrowserNodeAPI.transport.eventConditionalTransferCreated(payload)
        }
        function eventConditionalTransferResolved(payload) {
            connextBrowserNodeAPI.transport.eventConditionalTransferResolved(payload)
        }
    }
}
