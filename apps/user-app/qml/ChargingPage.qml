import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Loader {
  objectName: mobile.page === 'settlement' ? 'settlementPage' : 'chargingPage'
  active: mobile.activeOrder.id !== undefined
  sourceComponent: Item {
    id: screen
    readonly property var order: mobile.activeOrder
    readonly property string stateName: order.status
    readonly property real stateOfCharge: order.soc
    Flickable {
      anchors.fill: parent
      anchors.bottomMargin: actionDock.height
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
        Column {
          width: parent.width
          spacing: Theme.space
          AppText {
            width: parent.width
            text: {
              if (screen.stateName === 'reserved')
                return '已预约'
              if (screen.stateName === 'charging')
                return '正在充电'
              return '充电已结束'
            }
            font.pixelSize: Theme.titleSize
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
          }
          AppText {
            width: parent.width
            text: screen.order.stationName
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.bodySize
            color: Theme.muted
            elide: Text.ElideRight
          }
        }
        Item {
          width: parent.width
          height: 200
          Rectangle {
            anchors.centerIn: parent
            width: 176
            height: 176
            radius: 88
            color: Theme.secondaryLight
          }
          Canvas {
            id: progress
            anchors.centerIn: parent
            width: 200
            height: 200
            property real value: screen.stateName === 'reserved' ? 1 : Math.min(1, screen.stateOfCharge / 100)
            Behavior on value { NumberAnimation { duration: 500; easing.type: Easing.OutCubic } }
            onValueChanged: requestPaint()
            Connections {
              target: appearance
              function onChanged() { progress.requestPaint() }
            }
            onPaint: {
              var ctx = getContext('2d')
              ctx.clearRect(0, 0, width, height)
              ctx.lineWidth = 8
              ctx.strokeStyle = Theme.border
              ctx.beginPath()
              ctx.arc(width / 2, height / 2, 92, 0, Math.PI * 2)
              ctx.stroke()
              ctx.strokeStyle = Theme.primary
              ctx.lineCap = 'round'
              ctx.beginPath()
              ctx.arc(width / 2, height / 2, 92, -Math.PI / 2, Math.PI * 2 * value - Math.PI / 2)
              ctx.stroke()
            }
          }
          Column {
            anchors.centerIn: parent
            spacing: Theme.space
            AppText {
              anchors.horizontalCenter: parent.horizontalCenter
              text: screen.stateName === 'reserved' ? '预约剩余时间' : '电池电量'
              color: Theme.muted
              font.pixelSize: Theme.labelSize
            }
            AppText {
              anchors.horizontalCenter: parent.horizontalCenter
              objectName: 'reservationCountdown'
              visible: screen.stateName === 'reserved'
              text: mobile.reservationRemaining
              color: Theme.primaryText
              font.pixelSize: 40
              font.weight: Font.DemiBold
            }
            RollingNumber {
              anchors.horizontalCenter: parent.horizontalCenter
              visible: screen.stateName !== 'reserved'
              value: screen.stateOfCharge
              suffix: '%'
              color: Theme.primaryText
              font.pixelSize: 40
              font.weight: Font.DemiBold
            }
          }
        }
        Rectangle {
          width: parent.width
          height: metrics.implicitHeight + Theme.cardPadding * 2
          radius: Theme.cardRadius
          color: Theme.card
          Row {
            id: metrics
            x: Theme.cardPadding
            y: Theme.cardPadding
            width: parent.width - Theme.cardPadding * 2
            Repeater {
              model: [{label: '已充电量', key: 'energyKwh', scale: 1, decimals: 2, unit: '度'},
                      {label: '充电时长', key: 'durationSeconds', scale: 60, decimals: 0, unit: '分钟'},
                      {label: '当前费用', key: 'amountCents', scale: 100, decimals: 2, unit: '元'}]
              delegate: Column {
                required property var modelData
                width: metrics.width / 3
                spacing: Theme.microSpace
                AppText {
                  anchors.horizontalCenter: parent.horizontalCenter
                  text: modelData.label
                  font.pixelSize: Theme.labelSize
                  color: Theme.muted
                }
                RollingNumber {
                  anchors.horizontalCenter: parent.horizontalCenter
                  value: modelData.key === 'durationSeconds' ? Math.floor(screen.order[modelData.key] / modelData.scale) : screen.order[modelData.key] / modelData.scale
                  decimals: modelData.decimals
                  color: Theme.ink
                  font.pixelSize: Theme.titleSize
                  font.weight: Font.DemiBold
                }
                AppText {
                  anchors.horizontalCenter: parent.horizontalCenter
                  text: modelData.unit
                  font.pixelSize: Theme.labelSize
                  color: Theme.muted
                }
              }
            }
          }
        }
        Rectangle {
          width: parent.width
          height: detail.height + Theme.cardPadding * 2
          radius: Theme.cardRadius
          color: Theme.card
          Column {
            id: detail
            x: Theme.cardPadding
            y: Theme.cardPadding
            width: parent.width - Theme.cardPadding * 2
            spacing: Theme.space
            KeyValue {
              width: parent.width
              label: '充电桩'
              value: screen.order.chargerCode
            }
            KeyValue {
              width: parent.width
              label: '充电类型'
              value: (screen.order.chargerType === 'dc' ? '直流快充' : '交流慢充') + ' · ' + screen.order.powerKw + ' kW'
            }
            KeyValue {
              width: parent.width
              label: '电价'
              value: '¥' + (screen.order.priceCents / 100).toFixed(2) + ' / 度'
            }
            KeyValue {
              width: parent.width
              label: '开始时间'
              value: mobile.formatTime(screen.order.startedAt || '')
            }
            KeyValue {
              width: parent.width
              label: '钱包余额'
              value: '¥' + (mobile.user.balanceCents / 100).toFixed(2)
              valueColor: Theme.primaryText
            }
          }
        }
        AppText {
          width: parent.width - Theme.cardPadding * 2
          anchors.horizontalCenter: parent.horizontalCenter
          visible: screen.stateName === 'reserved' || screen.stateName === 'charging' || !!screen.order.stopReason
          text: {
            if (screen.stateName === 'reserved')
              return '预约超时自动取消'
            if (screen.stateName === 'charging')
              return '退出后仍计费，充满或余额用尽时自动结束。'
            return screen.order.stopReason
          }
          font.pixelSize: Theme.labelSize
          color: Theme.muted
          wrapMode: Text.WordWrap
          lineHeight: 1.5
        }
      }
    }
    Rectangle {
      id: actionDock
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      height: actionColumn.height + Theme.pagePadding * 2
      color: Theme.paper
      Rectangle {
        width: parent.width
        height: 1
        color: Theme.border
      }
      Column {
        id: actionColumn
        x: Theme.pagePadding
        y: Theme.pagePadding
        width: parent.width - Theme.pagePadding * 2
        spacing: Theme.space
        RowLayout {
          width: parent.width
          visible: screen.stateName === 'reserved'
          spacing: Theme.controlGap
          ActionButton {
            objectName: 'cancelReservationButton'
            Layout.preferredWidth: 80
            enabled: !mobile.busy
            text: '取消预约'
            variant: 'text'
            onClicked: {
              confirmation.action = 'cancel'
              confirmation.open()
            }
          }
          ActionButton {
            objectName: 'startChargingButton'
            Layout.fillWidth: true
            enabled: !mobile.busy
            text: '已连接，开始充电'
            onClicked: mobile.startCharging()
          }
        }
        ActionButton {
          objectName: 'stopChargingButton'
          width: parent.width
          visible: screen.stateName === 'charging'
          enabled: !mobile.busy
          text: '结束充电，前往结算'
          onClicked: {
            confirmation.action = 'stop'
            confirmation.open()
          }
        }
        ActionButton {
          objectName: 'settleOrderButton'
          width: parent.width
          visible: screen.stateName === 'pending_payment'
          enabled: !mobile.busy
          text: '确认支付 ¥' + (screen.order.amountCents / 100).toFixed(2)
          onClicked: mobile.settle()
        }
      }
    }
    Popup {
      id: confirmation
      property string action: 'stop'
      anchors.centerIn: Overlay.overlay
      width: screen.width - Theme.pagePadding * 2
      padding: Theme.cardPadding
      modal: true
      background: Rectangle {
        color: Theme.card
        radius: Theme.heroRadius
      }
      Overlay.modal: Rectangle {
        color: Theme.overlay
      }
      contentItem: Column {
        spacing: Theme.cardPadding
        AppText {
          width: parent.width
          text: confirmation.action === 'stop' ? '结束本次充电？' : '取消本次预约？'
          font.pixelSize: Theme.titleSize
          font.weight: Font.DemiBold
          wrapMode: Text.WordWrap
        }
        AppText {
          width: parent.width
          text: confirmation.action === 'stop' ? '按已充电量结算。' : '取消预约不收费。'
          color: Theme.muted
          wrapMode: Text.WordWrap
          lineHeight: 1.5
        }
        ActionButton {
          objectName: 'confirmOrderActionButton'
          width: parent.width
          text: confirmation.action === 'stop' ? '确认结束' : '确认取消'
          onClicked: {
            confirmation.close()
            if (confirmation.action === 'stop')
              mobile.stopCharging()
            else
              mobile.cancelReservation()
          }
        }
        ActionButton {
          width: parent.width
          text: confirmation.action === 'stop' ? '继续充电' : '保留预约'
          variant: 'text'
          onClicked: confirmation.close()
        }
      }
    }
  }
}
