import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Loader {
  objectName: 'profilePage'
  active: mobile.signedIn
  sourceComponent: Flickable {
    id: screen
    contentWidth: width
    contentHeight: content.height + Theme.pagePadding
    boundsBehavior: Flickable.DragOverBounds
    PullToRefresh { view: screen }
    clip: true
    ScrollBar.vertical: ScrollBar {
      policy: ScrollBar.AsNeeded
    }
    Column {
      id: content
      x: Theme.pagePadding
      y: 0
      width: parent.width - Theme.pagePadding * 2
      spacing: Theme.cardPadding
      Button {
        id: profileButton
        objectName: 'editProfileButton'
        width: parent.width
        implicitHeight: Math.max(80, contentItem.implicitHeight + topPadding + bottomPadding)
        padding: Theme.cardPadding
        topPadding: Theme.space
        bottomPadding: Theme.space
        Accessible.name: '修改个人信息，' + mobile.user.nickname
        onClicked: mobile.navigate('editProfile')
        background: Item {}
        contentItem: RowLayout {
          spacing: Theme.cardPadding
          Rectangle {
            Layout.preferredWidth: 64
            Layout.preferredHeight: 64
            radius: Theme.heroRadius
            color: Theme.disabled
            Image {
              anchors.centerIn: parent
              width: mobile.avatarSource ? 48 : 24
              height: width
              source: mobile.avatarSource || appearance.iconSource(':/icons/user.svg', Theme.ink)
              fillMode: Image.PreserveAspectCrop
              opacity: mobile.avatarSource ? 1 : 0.65
            }
          }
          Column {
            Layout.fillWidth: true
            spacing: Theme.microSpace
            AppText {
              width: parent.width
              text: mobile.user.nickname
              font.pixelSize: Theme.titleSize
              font.weight: Font.DemiBold
              wrapMode: Text.Wrap
            }
            AppText {
              text: mobile.user.phone
              color: Theme.muted
            }
          }
          AppIcon {
            name: 'chevron-right'
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
          }
        }
      }
      Rectangle {
        width: parent.width
        height: walletContent.height + Theme.cardPadding * 2
        radius: Theme.heroRadius
        color: Theme.surfaceDark
        Column {
          id: walletContent
          x: Theme.cardPadding
          y: Theme.cardPadding
          width: parent.width - Theme.cardPadding * 2
          spacing: Theme.space
          RowLayout {
            width: parent.width
            spacing: Theme.space
            AppText {
              Layout.fillWidth: true
              text: '钱包余额'
              font.pixelSize: 18
              font.weight: Font.Medium
              color: Theme.heroMuted
            }
            ActionButton {
              id: rechargeButton
              objectName: 'walletRechargeButton'
              Layout.preferredWidth: 80
              text: '充值'
              variant: 'primary'
              background: Rectangle {
                radius: height / 2
                color: rechargeButton.down ? Theme.primaryPressed : Theme.primary
              }
              onClicked: mobile.navigate('recharge')
            }
          }
          MoneyText {
            objectName: 'profileWalletBalance'
            width: parent.width
            fitToWidth: true
            cents: mobile.user.balanceCents
            valueColor: 'white'
            valueSize: 40
          }
        }
      }
      Rectangle {
        width: parent.width
        height: statsRow.implicitHeight + Theme.cardPadding * 2
        radius: Theme.cardRadius
        color: Theme.card
        RowLayout {
          id: statsRow
          x: Theme.space
          y: Theme.cardPadding
          width: parent.width - Theme.space * 2
          spacing: 0
          readonly property real numberWidth: width / 3 - Theme.space * 2
          readonly property int numberSize: {
            var values = mobile.profileStats
            var strings = [values.orderCount === undefined ? '—' : Number(values.orderCount).toFixed(0),
                           values.totalEnergyKwh === undefined ? '—' : Number(values.totalEnergyKwh).toFixed(1),
                           values.totalSpentCents === undefined ? '—' : (Number(values.totalSpentCents) / 100).toFixed(2)]
            var widest = Math.max.apply(null, strings.map(function (text) { return numberMetrics.advanceWidth(text) }))
            return Math.max(1, Math.floor(Theme.titleSize * Math.min(1, numberWidth / Math.max(1, widest))))
          }
          FontMetrics {
            id: numberMetrics
            font.pixelSize: Theme.titleSize
            font.weight: Font.DemiBold
          }
          Repeater {
            model: [
              {key: 'orderCount', label: '充电次数', decimals: 0, divisor: 1},
              {key: 'totalEnergyKwh', label: '累计电量（度）', decimals: 1, divisor: 1},
              {key: 'totalSpentCents', label: '累计实付（元）', decimals: 2, divisor: 100}
            ]
            delegate: Item {
              id: statistic
              required property var modelData
              required property int index
              Layout.fillWidth: true
              Layout.preferredWidth: 1
              implicitHeight: 32 + Theme.space + 36
              Column {
                id: statisticContent
                width: parent.width
                spacing: Theme.space
                AppText {
                  objectName: 'profileStat_' + statistic.modelData.key
                  width: parent.width - Theme.space * 2
                  anchors.horizontalCenter: parent.horizontalCenter
                  text: {
                    var value = mobile.profileStats[statistic.modelData.key]
                    return value === undefined ? '—' : (Number(value) / statistic.modelData.divisor).toFixed(statistic.modelData.decimals)
                  }
                  height: 32
                  font.pixelSize: statsRow.numberSize
                  font.weight: Font.DemiBold
                  wrapMode: Text.NoWrap
                  verticalAlignment: Text.AlignVCenter
                  horizontalAlignment: Text.AlignHCenter
                }
                AppText {
                  width: parent.width
                  height: 36
                  text: statistic.modelData.label
                  wrapMode: Text.Wrap
                  maximumLineCount: 2
                  lineHeightMode: Text.FixedHeight
                  lineHeight: 18
                  verticalAlignment: Text.AlignTop
                  color: Theme.muted
                  font.pixelSize: Theme.labelSize
                  horizontalAlignment: Text.AlignHCenter
                }
              }
              Rectangle {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 1
                height: 28
                color: Theme.border
                visible: statistic.index < 2
              }
            }
          }
        }
      }
      Rectangle {
        width: parent.width
        height: frozenText.implicitHeight + Theme.cardPadding * 2
        visible: mobile.user.status === 'frozen'
        radius: Theme.cardRadius
        color: Theme.dangerLight
        AppText {
          id: frozenText
          x: Theme.cardPadding
          y: Theme.cardPadding
          width: parent.width - Theme.cardPadding * 2
          text: '账号已冻结，无法预约或开始充电。已有订单可结束并结算。请联系管理员解冻。'
          wrapMode: Text.WordWrap
          color: Theme.danger
          lineHeight: 1.5
        }
      }
      Rectangle {
        width: parent.width
        height: profileOptions.height
        radius: Theme.cardRadius
        color: Theme.card
        clip: true
        Column {
          id: profileOptions
          width: parent.width
          MenuRow {
            width: parent.width
            title: '当前位置'
            description: mobile.locationName
            iconName: 'map-pin'
            onClicked: mobile.openLocationPicker()
          }
          RowLayout {
            x: Theme.cardPadding
            width: parent.width - Theme.cardPadding * 2
            height: 64
            spacing: Theme.cardPadding
            AppIcon {
              name: 'settings'
              Layout.preferredWidth: 24
              Layout.preferredHeight: 24
            }
            AppText {
              Layout.fillWidth: true
              text: '深色模式'
              font.pixelSize: Theme.bodyLargeSize
              font.weight: Font.Medium
            }
            Switch {
              id: themeSwitch
              objectName: 'profileThemeSwitch'
              Layout.preferredWidth: Theme.touchSize
              Layout.preferredHeight: Theme.touchSize
              Layout.minimumWidth: Theme.touchSize
              Layout.minimumHeight: Theme.touchSize
              padding: 0
              spacing: 0
              checked: appearance.dark
              Accessible.name: '深色模式'
              onToggled: appearance.mode = checked ? 'dark' : 'light'
              contentItem: Item {}
              indicator: Rectangle {
                anchors.centerIn: parent
                width: 48
                height: 28
                radius: height / 2
                color: themeSwitch.checked ? Theme.primary : Theme.disabled
                border.width: themeSwitch.visualFocus ? 2 : 0
                border.color: Theme.ink
                Rectangle {
                  x: themeSwitch.checked ? parent.width - width - 2 : 2
                  y: 2
                  width: 24
                  height: 24
                  radius: width / 2
                  color: Theme.primaryForeground
                  Behavior on x { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                }
              }
              background: Item {}
            }
          }
          MenuRow {
            objectName: 'profileHelpButton'
            width: parent.width
            title: '使用帮助'
            iconName: 'list'
            onClicked: {
              information.title = '使用帮助'
              information.body = '找电站：点击右上角位置可在地图上选点，点击刷新图标可重新获取系统定位。下拉页面可刷新电站信息。\n\n导航：点击电站的导航入口，使用高德地图规划真实路线；未安装高德时打开网页版。\n\n充电：预约后需在 15 分钟内开始充电，结束后按实际充电量结算。充值为模拟操作，不会实际扣款。\n\n个人统计：充电次数和累计电量包含已结束、待支付或已支付的充电；累计实付仅统计钱包实际扣除的充电费用。\n\n个人信息：点击头像或昵称可修改资料、选择和裁切头像，也可退出登录。\n\n外观：深色模式可在此切换；设置中可恢复跟随系统或更换主色。'
              information.open()
            }
          }
          Rectangle {
            x: Theme.cardPadding
            width: parent.width - Theme.cardPadding * 2
            height: 1
            color: Theme.border
          }
          MenuRow {
            objectName: 'settingsButton'
            width: parent.width
            title: '设置'
            iconName: 'settings'
            onClicked: mobile.navigate('settings')
          }
          MenuRow {
            objectName: 'profileAboutButton'
            width: parent.width
            title: '关于智充出行'
            iconName: 'circle-check'
            onClicked: {
              information.title = '关于智充出行'
              information.body = '智充出行\n\n电站查询、预约充电与订单管理。支持多端共享电站、订单和账户状态。\n\n导航使用真实地图服务，系统定位需要位置权限。当前联调环境的充值和充电数据用于功能测试。'
              information.open()
            }
          }
        }
      }
    }
    Popup {
      id: information
      parent: Overlay.overlay
      objectName: title === '使用帮助' ? 'helpPopup' : 'aboutPopup'
      property string title: ''
      property string body: ''
      anchors.centerIn: Overlay.overlay
      width: Math.min(screen.width - Theme.pagePadding * 2, 440)
      height: Math.min(informationContent.implicitHeight + padding * 2, screen.height - Theme.pagePadding * 2)
      padding: Theme.cardPadding
      modal: true
      background: Rectangle { radius: Theme.heroRadius; color: Theme.card }
      Overlay.modal: Rectangle { color: Theme.overlay }
      contentItem: ColumnLayout {
        id: informationContent
        spacing: Theme.cardPadding
        AppText {
          Layout.fillWidth: true
          text: information.title
          font.pixelSize: Theme.titleSize
          font.weight: Font.DemiBold
          wrapMode: Text.Wrap
        }
        Flickable {
          Layout.fillWidth: true
          Layout.fillHeight: true
          implicitHeight: informationText.implicitHeight
          contentWidth: width
          contentHeight: informationText.implicitHeight
          clip: true
          boundsBehavior: Flickable.StopAtBounds
          ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
          AppText {
            id: informationText
            width: parent.width
            text: information.body
            color: Theme.muted
            wrapMode: Text.Wrap
            lineHeight: 1.5
          }
        }
        ActionButton {
          Layout.fillWidth: true
          text: '知道了'
          onClicked: information.close()
        }
      }
    }
  }
}
