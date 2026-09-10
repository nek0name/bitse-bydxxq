import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Flickable {
  id: screen
  objectName: 'homePage'
  contentWidth: width
  contentHeight: content.height + Theme.pagePadding
  PullToRefresh { view: screen }
  clip: true
  boundsBehavior: Flickable.DragOverBounds
  readonly property var suggestion: mobile.stations.find(function (station) {
      return station.recommended
    })
  ScrollBar.vertical: ScrollBar {
    policy: ScrollBar.AsNeeded
  }
  Column {
    id: content
    x: Theme.pagePadding
    y: 0
    width: parent.width - Theme.pagePadding * 2
    spacing: Theme.cardPadding
    Column {
      id: searchControls
      width: parent.width
      spacing: Theme.cardPadding
      AppField {
        objectName: 'stationSearchInput'
        implicitHeight: Theme.touchSize
        background: Rectangle { color: Theme.card; radius: 12 }
        width: parent.width
        placeholderText: '搜索电站名称 / 地址'
        text: mobile.query
        onTextEdited: {
          mobile.query = text
          searchDebounce.restart()
        }
        onAccepted: {
          searchDebounce.stop()
          mobile.refreshStations()
          focus = false
        }
        Timer {
          id: searchDebounce
          interval: 350
          onTriggered: mobile.refreshStations()
        }
      }
      RowLayout {
        width: parent.width
        spacing: Theme.space
        Item {
          Layout.fillWidth: true
          implicitHeight: Theme.touchSize
          Rectangle {
            anchors.centerIn: parent
            width: parent.width
            height: 40
            radius: height / 2
            color: Theme.card
            border.color: Theme.border
          }
          Row {
            anchors.fill: parent
            anchors.leftMargin: Theme.microSpace
            anchors.rightMargin: Theme.microSpace
            Repeater {
              model: [{key: 'distance', text: '距离', label: '距离优先'},
                      {key: 'price', text: '价格', label: '价格最低'},
                      {key: 'idle', text: '空闲', label: '空闲最多'}]
              delegate: ActionButton {
                id: sortButton
                required property var modelData
                objectName: 'sort_' + modelData.key
                width: parent.width / 3
                height: Theme.touchSize
                text: modelData.text
                Accessible.name: modelData.label
                horizontalPadding: Theme.microSpace
                topPadding: Theme.microSpace
                bottomPadding: Theme.microSpace
                variant: 'chip'
                selected: mobile.sort === modelData.key
                textColor: selected ? Theme.primaryForeground : Theme.muted
                background: Item {
                  Rectangle {
                    anchors.centerIn: parent
                    width: parent.width
                    height: 32
                    radius: height / 2
                    color: sortButton.selected ? Theme.primary : sortButton.down ? Theme.primaryLight : 'transparent'
                  }
                }
                onClicked: mobile.sort = modelData.key
              }
            }
          }
        }
        ActionButton {
          objectName: 'fastOnlyButton'
          Layout.preferredWidth: 64
          text: '仅快充'
          horizontalPadding: Theme.space
          variant: 'chip'
          selected: mobile.fastOnly
          onClicked: mobile.fastOnly = !mobile.fastOnly
        }
      }
    }
    Loader {
      width: parent.width
      active: mobile.activeOrder.id !== undefined
      sourceComponent: Button {
        id: activeBanner
        objectName: 'activeOrderBanner'
        implicitHeight: activeInfo.implicitHeight + Theme.cardPadding * 2
        padding: Theme.cardPadding
        Accessible.name: '处理' + mobile.statusLabel(mobile.activeOrder.status) + '订单'
        onClicked: mobile.openActiveOrder()
        background: Rectangle {
          radius: Theme.cardRadius
          color: activeBanner.down ? Theme.amberPressed : Theme.amberLight
        }
        contentItem: RowLayout {
          spacing: Theme.space
          Column {
            id: activeInfo
            Layout.fillWidth: true
            spacing: Theme.microSpace
            AppText {
              text: '待处理：' + mobile.statusLabel(mobile.activeOrder.status) + '订单'
              font.weight: Font.Medium
              color: Theme.amber
            }
            AppText {
              text: mobile.activeOrder.stationName
              width: parent.width
              wrapMode: Text.Wrap
              font.pixelSize: Theme.labelSize
              color: Theme.amber
            }
          }
          AppIcon {
            name: 'chevron-right'
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
          }
        }
      }
    }
    Loader {
      objectName: 'recommendedStationLoader'
      width: parent.width
      active: screen.suggestion !== undefined
      visible: active
      sourceComponent: StationCard {
        stationData: screen.suggestion
        highlighted: true
      }
    }
    Repeater {
      model: mobile.stations
      delegate: StationCard {
        required property var modelData
        width: content.width
        stationData: modelData
      }
    }
    EmptyState {
      width: parent.width
      objectName: 'stationEmptyState'
      visible: mobile.stations.length === 0 && !mobile.loadingStations && !mobile.error
      title: '没有找到电站'
      description: '试试其他位置或筛选条件'
    }
  }
}
