import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Loader {
  objectName: 'stationPage'
  active: mobile.station.id !== undefined
  sourceComponent: Flickable {
    contentWidth: width
    contentHeight: content.height + Theme.pagePadding * 2
    boundsBehavior: Flickable.StopAtBounds
    clip: true
    ScrollBar.vertical: ScrollBar {
      policy: ScrollBar.AsNeeded
    }
    Column {
      id: content
      x: Theme.pagePadding
      y: Theme.pagePadding
      width: parent.width - Theme.pagePadding * 2
      spacing: Theme.cardPadding
      Rectangle {
        width: parent.width
        height: header.height + Theme.cardPadding * 2
        radius: Theme.heroRadius
        color: Theme.surfaceDark
        Column {
          id: header
          x: Theme.cardPadding
          y: Theme.cardPadding
          width: parent.width - Theme.cardPadding * 2
          spacing: Theme.cardPadding
          Badge {
            text: mobile.station.region
            visible: text.length > 0
            maximumWidth: parent.width
            fill: Theme.surfaceDarkRaised
            textColor: 'white'
          }
          Column {
            width: parent.width
            spacing: Theme.space
            AppText {
              width: parent.width
              text: mobile.station.name
              font.pixelSize: Theme.titleSize
              font.weight: Font.DemiBold
              color: 'white'
              wrapMode: Text.WordWrap
            }
            AppText {
              width: parent.width
              text: mobile.station.address
              color: Theme.heroMuted
              font.pixelSize: Theme.bodySize
              wrapMode: Text.WordWrap
              lineHeight: 1.4
            }
          }
          RowLayout {
            width: parent.width
            spacing: Theme.cardPadding
            Column {
              Layout.fillWidth: true
              spacing: Theme.microSpace
              MoneyText {
                cents: mobile.station.priceCents
                valueColor: Theme.accent
              }
              AppText {
                text: '电价（元/度）'
                color: Theme.heroMuted
                font.pixelSize: Theme.labelSize
              }
            }
            Column {
              Layout.fillWidth: true
              spacing: Theme.microSpace
              AppText {
                text: mobile.station.idleChargers + ' / ' + mobile.station.totalChargers
                color: 'white'
                font.pixelSize: Theme.headlineSize
                font.weight: Font.DemiBold
              }
              AppText {
                text: '空闲电桩 / 全部'
                color: Theme.heroMuted
                font.pixelSize: Theme.labelSize
              }
            }
          }
          ActionButton {
            objectName: 'stationNavigationButton'
            width: parent.width
            variant: 'text'
            textColor: Theme.heroMuted
            text: '直线 ' + mobile.station.distanceKm.toFixed(1) + ' km · 导航'
            onClicked: mobile.openNavigation(mobile.station)
          }
        }
      }
      Rectangle {
        width: parent.width
        height: forecastNote.implicitHeight + Theme.cardPadding * 2
        visible: !!mobile.station.forecastAt
        radius: Theme.cardRadius
        color: Theme.primaryLight
        AppText {
          id: forecastNote
          x: Theme.cardPadding
          y: Theme.cardPadding
          width: parent.width - Theme.cardPadding * 2
          text: '1 小时后预计空闲 ' + mobile.station.predictedAvailableChargers + ' 桩'
          font.pixelSize: Theme.bodySize
          color: Theme.primaryText
          wrapMode: Text.WordWrap
          lineHeight: 1.5
        }
      }
      AppText {
        text: '选择充电桩'
        font.pixelSize: Theme.bodyLargeSize
        font.weight: Font.Medium
      }
      Repeater {
        model: mobile.chargers
        delegate: Rectangle {
          id: chargerCard
          required property var modelData
          objectName: 'chargerCard_' + modelData.id
          width: content.width
          height: chargerContent.height + Theme.cardPadding * 2
          radius: Theme.cardRadius
          color: Theme.card

          Column {
            id: chargerContent
            x: Theme.cardPadding
            y: Theme.cardPadding
            width: parent.width - Theme.cardPadding * 2
            spacing: Theme.controlGap
            RowLayout {
              width: parent.width
              spacing: Theme.controlGap
              Rectangle {
                Layout.preferredWidth: 48
                Layout.preferredHeight: 48
                radius: Theme.cardRadius
                color: Theme.primaryLight
                AppIcon {
                  anchors.centerIn: parent
                  name: 'zap'
                }
              }
              Column {
                Layout.fillWidth: true
                spacing: Theme.microSpace
                AppText {
                  width: parent.width
                  text: chargerCard.modelData.code
                  font.pixelSize: Theme.bodyLargeSize
                  font.weight: Font.Medium
                  elide: Text.ElideRight
                }
                AppText {
                  width: parent.width
                  text: (chargerCard.modelData.type === 'dc' ? '直流快充' : '交流慢充') + ' · ' + chargerCard.modelData.powerKw + ' kW'
                  color: Theme.muted
                  font.pixelSize: Theme.labelSize
                  elide: Text.ElideRight
                }
              }
              ActionButton {
                objectName: 'reserveCharger_' + chargerCard.modelData.id
                Layout.preferredWidth: 96
                variant: 'text'
                textColor: Theme.primaryText
                text: '预约充电'
                visible: chargerCard.modelData.status === 'idle'
                enabled: chargerCard.modelData.status === 'idle' && !mobile.busy
                onClicked: mobile.reserve(Number(chargerCard.modelData.id))
              }
            }
            Column {
              width: parent.width
              spacing: Theme.microSpace
              AppText {
                text: mobile.statusLabel(chargerCard.modelData.status)
                color: Theme.muted
                font.pixelSize: Theme.labelSize
              }
              AvailabilityBar {
                objectName: 'chargerAvailability_' + chargerCard.modelData.id
                width: parent.width
                total: 1
                available: chargerCard.modelData.status === 'idle' ? 1 : 0
                faults: chargerCard.modelData.status === 'fault' || chargerCard.modelData.status === 'restarting' ? 1 : 0
              }
            }
          }
        }
      }
      EmptyState {
        width: parent.width
        visible: mobile.chargers.length === 0 && !mobile.error
        title: '暂无充电桩'
      }
      AppText {
        width: parent.width
        text: '预约保留 15 分钟，按充电量计费。'
        color: Theme.muted
        font.pixelSize: Theme.labelSize
        wrapMode: Text.WordWrap
        lineHeight: 1.4
      }
    }
  }
}
