import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
  id: card
  required property var stationData
  objectName: 'stationCard_' + stationData.id
  padding: Theme.cardPadding
  implicitHeight: info.implicitHeight + topPadding + bottomPadding
  Accessible.name: stationData.name + '，空闲 ' + stationData.idleChargers + ' 个充电桩，查看详情'
  onClicked: mobile.openStation(stationData.id)
  background: Rectangle {
    radius: Theme.cardRadius
    color: card.down || card.visualFocus ? Theme.primaryLight : Theme.card
  }
  contentItem: Column {
    id: info
    spacing: Theme.cardPadding
    RowLayout {
      width: parent.width
      spacing: Theme.controlGap
      Rectangle {
        Layout.preferredWidth: 48
        Layout.preferredHeight: 48
        radius: Theme.cardRadius
        color: card.highlighted ? Theme.accent : Theme.primaryLight
        AppIcon {
          anchors.centerIn: parent
          name: 'zap'
          color: card.highlighted ? Theme.surfaceDark : Theme.ink
        }
      }
      Column {
        Layout.fillWidth: true
        spacing: Theme.microSpace
        AppText {
          width: parent.width
          text: card.stationData.name
          font.pixelSize: Theme.bodyLargeSize
          font.weight: Font.Medium
          elide: Text.ElideRight
        }
        AppText {
          width: parent.width
          text: card.stationData.address
          font.pixelSize: Theme.labelSize
          color: Theme.muted
          elide: Text.ElideRight
        }
      }
      Button {
        id: navigation
        objectName: 'navigateStation_' + card.stationData.id
        Layout.preferredWidth: Math.max(Theme.touchSize, distanceText.implicitWidth)
        Layout.preferredHeight: Theme.touchSize
        Layout.alignment: Qt.AlignRight | Qt.AlignTop
        padding: 0
        Accessible.name: '导航到' + card.stationData.name + '，直线距离' + card.stationData.distanceKm.toFixed(1) + '公里'
        onClicked: mobile.openNavigation(card.stationData)
        background: Rectangle {
          radius: 8
          color: navigation.down ? Theme.primarySoftPressed : navigation.hovered || navigation.visualFocus ? Theme.primaryLight : 'transparent'
        }
        contentItem: Column {
          spacing: Theme.microSpace
          AppIcon {
            name: 'navigation'
            anchors.right: parent.right
          }
          AppText {
            id: distanceText
            anchors.right: parent.right
            text: card.stationData.distanceKm.toFixed(1) + ' km'
            font.pixelSize: Theme.labelSize
            color: Theme.muted
          }
        }
      }
    }
    Flow {
      width: parent.width
      spacing: Theme.space
      visible: card.highlighted && card.stationData.predictedAvailableChargers >= 0
      Badge {
        text: '1 小时后预计空闲 ' + card.stationData.predictedAvailableChargers + ' 桩'
        fill: Theme.paper
        textColor: Theme.muted
      }
    }
    Column {
      width: parent.width
      spacing: Theme.microSpace
      RowLayout {
        width: parent.width
        spacing: Theme.cardPadding
        MoneyText {
          objectName: 'stationPrice'
          cents: card.stationData.priceCents
          suffix: '/度'
          Layout.alignment: Qt.AlignBaseline
        }
        Item {
          Layout.fillWidth: true
        }
        AppText {
          text: '空闲 ' + card.stationData.idleChargers + ' / ' + card.stationData.totalChargers + ' 桩'
          font.pixelSize: Theme.labelSize
          color: Theme.muted
          Layout.alignment: Qt.AlignRight | Qt.AlignBaseline
        }
      }
      AvailabilityBar {
        objectName: 'stationAvailability_' + card.stationData.id
        width: parent.width
        total: card.stationData.totalChargers
        available: card.stationData.idleChargers
        faults: card.stationData.faultChargers
      }
    }
  }
}
