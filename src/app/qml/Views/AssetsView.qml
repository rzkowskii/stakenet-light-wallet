import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Item {

    ColumnLayout {
        anchors.fill: parent
        spacing: 30

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            spacing: 10

            XSNLabel {
                Layout.fillWidth: true
                text: "Show / Hide"
                font.pixelSize: 32
                font.family: fontMedium.name
            }

            XSNLabel {
                Layout.fillWidth: true
                text: "Hidden and disabled assets are not shown in the wallet. In order for an asset to be hidden, it must have a balance of 0. To use the DEX interface, a minimum of two assets must be enabled."
                font.pixelSize: 20
                wrapMode: Text.WordWrap
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Item {
                Layout.leftMargin: 70
                Layout.preferredHeight: 50
                Layout.fillWidth: true

                FadedRectangle {
                    id: fadeBackground
                    anchors.fill: parent
                    radius: 3
                    activeStateColor: SkinColors.headerBackground
                    inactiveStateColor: SkinColors.secondaryBackground
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 10

                    SearchTextField {
                        id: searchArea
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        placeholderText: "Search for assets ..."
                        validator: RegExpValidator {
                            regExp: /[a-zA-Z]{1,10}\D/g
                        }
                        onComponentActiveFocusChanged: {
                            if(componentActiveFocus)
                            {
                                fadeBackground.startFade()
                            }
                            else
                            {
                                fadeBackground.stopFade();
                            }
                        }
                    }

                    ColorOverlayImage {
                        Layout.alignment: Qt.AlignRight
                        Layout.rightMargin: 16
                        imageSize: 32
                        width: imageSize
                        height: imageSize
                        imageSource: "qrc:/images/magnifyingGlass.png"
                        color: SkinColors.magnifyingGlass
                    }
                }
            }

            AssetsListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: QMLSortFilterListProxyModel {
                    source: QMLSortFilterListProxyModel {
                        source: WalletAssetsListModel {
                            id: assetModel
                            Component.onCompleted: initialize(ApplicationViewModel)
                        }

                        filterRole: "isActive"
                        filterString: "1"
                    }
                    sortRole: "name"
                    filterRole: "nameSymbol"
                    filterString: searchArea.text
                    filterCaseSensitivity: Qt.CaseInsensitive
                    sortCaseSensitivity: Qt.CaseInsensitive
                }

                onAssetIsActiveToogled: {
                    assetModel.toogleAssetActiveState(id);
                }
            }
        }
    }
}
