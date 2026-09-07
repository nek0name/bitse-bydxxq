import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
  id: screen
  objectName: 'ordersPage'
  leftMargin: Theme.pagePadding
  rightMargin: Theme.pagePadding
  model: mobile.orders
  spacing: Theme.cardPadding
  clip: true
  reuseItems: true
  cacheBuffer: height / 2
  boundsBehavior: Flickable.DragOverBounds
  readonly property real pullDistance: Math.max(0, originY - contentY)
  Component.onCompleted: Qt.callLater(function () {
    contentY = originY + mobile.orderScrollPosition()
  })
  Component.onDestruction: mobile.saveOrderScroll(Math.max(0, contentY - originY))
  function fillViewport() {
    if (count > 0 && contentHeight < height && !mobile.loadingOrders && !mobile.error)
      mobile.loadMoreOrders()
  }
  onContentHeightChanged: Qt.callLater(fillViewport)
  onHeightChanged: Qt.callLater(fillViewport)
  Connections {
    target: mobile
    function onOrdersChanged() { Qt.callLater(screen.fillViewport) }
  }
  onDraggingChanged: {
    if (!dragging && pullDistance >= 48 && !mobile.loadingOrders) mobile.refresh()
  }
  onMovementEnded: {
    if (atYEnd && !mobile.error) mobile.loadMoreOrders()
  }
  onContentYChanged: {
    if (moving && !mobile.error && contentY + height >= contentHeight - height / 2)
      mobile.loadMoreOrders()
  }
  Rectangle {
    objectName: 'orderRefreshIndicator'
    parent: screen
    anchors.top: parent.top
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.topMargin: Theme.space
    width: refreshLabel.implicitWidth + Theme.cardPadding * 2
    height: 40
    radius: 12
    color: Theme.card
    z: 2
    visible: screen.count > 0 && (mobile.refreshingOrders || screen.dragging && screen.pullDistance > 8)
    AppText {
      id: refreshLabel
      anchors.centerIn: parent
      text: mobile.refreshingOrders ? '正在刷新…' : screen.pullDistance >= 48 ? '松手刷新' : '下拉刷新'
      color: Theme.muted
    }
  }
  ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
  header: Column {
    width: screen.width - Theme.pagePadding * 2
    topPadding: Theme.pagePadding
    bottomPadding: Theme.cardPadding
    Row {
      width: parent.width
      spacing: Theme.space
      Repeater {
        model: [{key: 'all', label: '全部订单'},
                {key: 'active', label: '进行中'},
                {key: 'paid', label: '已完成'}]
        delegate: ActionButton {
          required property var modelData
          width: (screen.width - Theme.pagePadding * 2 - Theme.space * 2) / 3
          text: modelData.label
          horizontalPadding: Theme.space
          variant: 'chip'
          selected: mobile.orderFilter === modelData.key
          onClicked: {
            if (mobile.orderFilter === modelData.key) return
            mobile.filterOrders(modelData.key)
            screen.positionViewAtBeginning()
          }
        }
      }
    }
  }
  delegate: Button {
    id: card
    required property var order
    objectName: 'orderCard_' + order.id
    width: screen.width - Theme.pagePadding * 2
    implicitHeight: details.implicitHeight + Theme.cardPadding * 2
    padding: Theme.cardPadding
    Accessible.name: order.stationName + '，' + mobile.statusLabel(order.status) + '，查看订单'
    onClicked: mobile.openOrder(order.id)
    background: Rectangle {
      radius: Theme.cardRadius
      color: card.down || card.visualFocus ? Theme.primaryLight : Theme.card
    }
    contentItem: Column {
      id: details
      spacing: Theme.cardPadding
      RowLayout {
        width: parent.width
        spacing: Theme.space
        AppText {
          Layout.fillWidth: true
          text: card.order.stationName
          font.pixelSize: Theme.bodyLargeSize
          font.weight: Font.Medium
          elide: Text.ElideRight
        }
        Badge {
          text: mobile.statusLabel(card.order.status)
          textColor: card.order.status === 'pending_payment' ? Theme.amber : card.order.status === 'cancelled' ? Theme.muted : Theme.primaryText
          fill: card.order.status === 'pending_payment' ? Theme.amberLight : card.order.status === 'cancelled' ? Theme.paper : Theme.primaryLight
        }
      }
      Column {
        width: parent.width
        spacing: Theme.microSpace
        AppText {
          text: mobile.formatTime(card.order.createdAt)
          color: Theme.muted
          font.pixelSize: Theme.labelSize
        }
        AppText {
          text: card.order.chargerCode + ' · ' + card.order.energyKwh.toFixed(2) + ' 度 · ' + Math.floor(card.order.durationSeconds / 60) + ' 分钟'
          color: Theme.muted
          font.pixelSize: Theme.labelSize
        }
      }
      Rectangle {
        width: parent.width
        height: 1
        color: Theme.border
      }
      RowLayout {
        width: parent.width
        MoneyText {
          Layout.fillWidth: true
          cents: card.order.amountCents
        }
        AppText {
          text: card.order.status === 'paid' || card.order.status === 'cancelled' ? '查看小票' : '继续处理'
          color: Theme.primaryText
        }
        AppIcon {
          name: 'chevron-right'
          Layout.preferredWidth: 24
          Layout.preferredHeight: 24
        }
      }
    }
  }
  footer: Column {
    width: screen.width - Theme.pagePadding * 2
    spacing: Theme.space
    topPadding: Theme.cardPadding
    bottomPadding: Theme.pagePadding
    BusyIndicator {
      anchors.horizontalCenter: parent.horizontalCenter
      visible: mobile.loadingOrders && (!mobile.refreshingOrders || screen.count === 0)
      running: visible
    }
    EmptyState {
      width: parent.width
      visible: screen.count === 0 && !mobile.loadingOrders && !mobile.error
      title: mobile.orderFilter === 'all' ? '还没有充电订单' : '暂无此类订单'
    }
    ActionButton {
      anchors.horizontalCenter: parent.horizontalCenter
      visible: mobile.hasMoreOrders && !mobile.loadingOrders && !mobile.error
      text: '加载更多'
      variant: 'text'
      onClicked: mobile.loadMoreOrders()
    }
  }
}
