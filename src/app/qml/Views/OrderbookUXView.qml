import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

ColumnLayout {
    spacing: 15

    Settings {
        id: settings
        category: "Dex"
        property string selectTypeOptionName: "Predictive"
        property string useCoinsForRemoteBalance: "Local currency"
        property string showTxType: "All"
    }

    ColumnLayout {
        spacing: 15

        XSNLabel {
            text: "Orderbook UX"
            font.pixelSize: 23
        }

        Column {
            id: buttonColumn
            Layout.fillWidth: true
            Layout.fillHeight: true

            Repeater {
                model: ListModel {
                    ListElement {
                        name: "Default"
                        description: "If the user clicks on an order in the orderbook, the order type will not change."
                    }
                    ListElement {
                        name: "Predictive"
                        description: "If the user clicks on an order in the orderbook, the order type will automatically change to the opposite order type. I.e.\nClick on a buy order, your order type will be a sell order."
                    }
                    ListElement {
                        name: "Matching"
                        description: "If the user clicks on an order in the orderbook, the order type will match the type of order clicked on. I.e.\nClick on a buy order, your order type will be a buy order."
                    }
                }

                delegate: RadioButtonDelegate {
                    height: 70
                    width: parent.width
                    name: model.name
                    description: model.description
                    radioGroup: radioOrderbookGroup
                    radioButton.checked: model.name === settings.selectTypeOptionName
                    radioButton.onClicked: settings.selectTypeOptionName = name
                }
            }

            ButtonGroup {
                id: radioOrderbookGroup
            }
        }
    }

    ColumnLayout {
        spacing: 15

        XSNLabel {
            text: "Can receive balance unit"
            font.pixelSize: 23
        }

        Column {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Repeater {
                model: ListModel {
                    ListElement {
                        name: "Coin"
                        description: "Can receive balance will be presented in coins."
                    }
                    ListElement {
                        name: "Local currency"
                        description: "Can receive balance will be presented in local currency."
                    }
                }

                delegate: RadioButtonDelegate {
                    height: 65
                    width: parent.width
                    name: model.name
                    description: model.description
                    radioGroup: radioCanReceiveUnitsGroup
                    radioButton.checked: model.name === settings.useCoinsForRemoteBalance
                    radioButton.onClicked: settings.useCoinsForRemoteBalance = model.name
                }
            }

            ButtonGroup {
                id: radioCanReceiveUnitsGroup
            }
        }
    }

    ColumnLayout {
        spacing: 15

        XSNLabel {
            text: "Transaction to display in wallet tab"
            font.pixelSize: 23
        }

        Column {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Repeater {
                model: ListModel {
                    ListElement {
                        name: "All"
                        description: "All transactions will be displayed in list view."
                    }
                    ListElement {
                        name: "Hide Dex transactions"
                        description: "Dex transactions will not be displayed in list view."
                    }
                }

                RadioButtonDelegate {
                                    height: 65
                                    width: parent.width
                                    name: model.name
                                    description: model.description
                                    radioGroup: radioTxtypeGroup
                                    radioButton.checked: model.name === settings.showTxType
                                    radioButton.onClicked: settings.showTxType = model.name
                                }

            }

            ButtonGroup {
                id: radioTxtypeGroup
            }
        }
    }
}
