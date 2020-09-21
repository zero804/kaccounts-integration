/*
 *   SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *   SPDX-FileCopyrightText: 2020 Dan Leinir Turthra Jensen <admin@leinir.dk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls
import QtQuick.Layouts 1.12

import org.kde.kirigami 2.4 as Kirigami
import org.kde.kcm 1.2

import org.kde.kaccounts 1.2 as KAccounts

ScrollViewKCM {
    id: root
    title: i18n("Add New Account")

    view: ListView {

        clip: true

        model: KAccounts.ProvidersModel {}
        delegate: Kirigami.AbstractListItem {
            id: accountDelegate
            width: ListView.view.width
            enabled: model.supportsMultipleAccounts === true || model.accountsCount === 0

            contentItem: RowLayout {
                implicitWidth: accountDelegate.ListView.view.width
                implicitHeight: Kirigami.Units.iconSizes.large + Kirigami.Units.smallSpacing * 2
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Icon {
                    source: model.iconName
                    Layout.preferredWidth: Kirigami.Units.iconSizes.large
                    Layout.preferredHeight: Kirigami.Units.iconSizes.large
                    Item {
                        visible: model.accountsCount > 0
                        anchors {
                            bottom: parent.bottom
                            right: parent.right
                        }
                        height: parent.height / 3
                        width: height
                        Rectangle {
                            anchors.fill: parent
                            radius: height / 2
                            color: Kirigami.Theme.highlightColor
                            border {
                                width: 1
                                color: Kirigami.Theme.highlightedTextColor
                            }
                        }
                        Controls.Label {
                            anchors.fill: parent
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: Kirigami.Theme.highlightedTextColor
                            text: model.accountsCount
                        }
                    }
                }
                Controls.Label {
                    Layout.fillWidth: true
                    text: model.displayName + "\n" + model.description;
                }
            }
            onClicked: {
                var job = jobComponent.createObject(root, { "providerName": model.name })
                job.start()
            }
        }
    }

    Component {
        id: jobComponent
        KAccounts.CreateAccountJob {
            onFinished: kcm.pop()
        }
    }
}
