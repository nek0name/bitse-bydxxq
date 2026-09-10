import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  id: root
  objectName: 'mobileRoot'
  width: 430
  height: 860
  color: Theme.paper
  readonly property bool refreshing: mobile.busy || (mobile.page !== 'orders' && mobile.loadingStations)
  readonly property bool isTab: mobile.page === 'home' || mobile.page === 'orders' || mobile.page === 'profile'
  readonly property var pageInfo: ({
      "login": {
        "title": '',
        "source": 'LoginPage.qml'
      },
      "home": {
        "title": '附近电站',
        "source": 'HomePage.qml'
      },
      "orders": {
        "title": '我的订单',
        "source": 'OrdersPage.qml'
      },
      "profile": {
        "title": '我的',
        "source": 'ProfilePage.qml'
      },
      "station": {
        "title": '电站详情',
        "source": 'StationPage.qml'
      },
      "charge": {
        "title": '充电服务',
        "source": 'ChargingPage.qml'
      },
      "settlement": {
        "title": '订单结算',
        "source": 'ChargingPage.qml'
      },
      "receipt": {
        "title": '充电小票',
        "source": 'ReceiptPage.qml'
      },
      "location": {
        "title": '选择当前位置',
        "source": 'LocationPage.qml'
      },
      "recharge": {
        "title": '钱包充值',
        "source": 'RechargePage.qml'
      },
      "settings": {
        "title": '设置',
        "source": 'SettingsPage.qml'
      },
      "editProfile": {
        "title": '个人信息',
        "source": 'EditProfilePage.qml'
      }
    })[mobile.page]

  ColumnLayout {
    anchors.fill: parent
    spacing: 0
    Item {
      objectName: 'mobileTopBar'
      Layout.fillWidth: true
      Layout.preferredHeight: visible ? Theme.topBarHeight : 0
      visible: mobile.page !== 'login'
      RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        spacing: Theme.space
        IconButton {
          objectName: 'backButton'
          visible: !root.isTab
          iconName: 'arrow-left'
          label: '返回上一页'
          enabled: !mobile.busy
          onClicked: mobile.back()
        }
        AppText {
          Layout.fillWidth: true
          text: root.pageInfo.title
          font.pixelSize: Theme.titleSize
          font.weight: Font.DemiBold
          wrapMode: Text.Wrap
        }
        RowLayout {
          id: locationControls
          visible: mobile.page === 'home'
          Layout.preferredWidth: parent.width * 0.6
          Layout.maximumWidth: parent.width * 0.6
          Layout.minimumWidth: Theme.touchSize * 2
          spacing: 0
          Button {
            id: headerLocation
            objectName: 'chooseLocationButton'
            Layout.fillWidth: true
            Layout.minimumWidth: Theme.touchSize
            implicitWidth: Theme.touchSize
            implicitHeight: Theme.touchSize
            padding: 4
            topPadding: 0
            bottomPadding: 0
            leftInset: 0
            rightInset: 0
            topInset: 0
            bottomInset: 0
            Accessible.name: '当前位置：' + mobile.locationName + '，在地图上选择位置'
            onClicked: mobile.openLocationPicker()
            background: Rectangle {
              radius: 12
              color: headerLocation.down ? Theme.primaryLight : 'transparent'
            }
            contentItem: AppText {
              id: locationLabel
              text: mobile.locating ? '定位中…' : mobile.locationName
              font.pixelSize: Theme.bodySize
              color: Theme.primaryText
              verticalAlignment: Text.AlignVCenter
              horizontalAlignment: Text.AlignRight
              wrapMode: Text.Wrap
              maximumLineCount: 2
              elide: Text.ElideRight
            }
          }
          IconButton {
            objectName: 'refreshLocationButton'
            iconName: 'refresh-cw'
            label: '刷新当前位置'
            enabled: !mobile.locating
            onClicked: mobile.refreshLocation()
          }
        }
      }
    }
    Item {
      id: pageLoader
      objectName: 'pageLoader'
      Layout.fillWidth: true
      Layout.fillHeight: true
      clip: true
      property bool frontIsA: true
      readonly property var item: frontIsA ? loaderA.item : loaderB.item
      readonly property int status: frontIsA ? loaderA.status : loaderB.status
      readonly property bool busy: pageTransition.running
      property var incoming: null
      property var outgoing: null
      function showPage() {
        pageTransition.stop()
        if (outgoing) outgoing.source = ''
        outgoing = frontIsA ? loaderA : loaderB
        incoming = frontIsA ? loaderB : loaderA
        incoming.source = root.pageInfo.source
        incoming.z = mobile.transitionDirection < 0 ? 0 : 1
        outgoing.z = mobile.transitionDirection < 0 ? 1 : 0
        incoming.x = mobile.transitionDirection < 0 ? -width * 0.25 : width
        outgoing.x = 0
        frontIsA = !frontIsA
        if (mobile.transitionDirection === 0) {
          incoming.x = 0
          outgoing.source = ''
          outgoing = null
        } else {
          pageTransition.restart()
        }
      }
      Loader {
        id: loaderA
        width: parent.width
        height: parent.height
        source: 'LoginPage.qml'
        visible: status === Loader.Ready
        Rectangle { anchors.fill: parent; color: Theme.paper; z: -1 }
      }
      Loader {
        id: loaderB
        width: parent.width
        height: parent.height
        visible: status === Loader.Ready
        Rectangle { anchors.fill: parent; color: Theme.paper; z: -1 }
      }
      ParallelAnimation {
        id: pageTransition
        NumberAnimation {
          target: pageLoader.incoming
          property: 'x'
          to: 0
          duration: 280
          easing.type: Easing.OutCubic
        }
        NumberAnimation {
          target: pageLoader.outgoing
          property: 'x'
          to: mobile.transitionDirection < 0 ? pageLoader.width : -pageLoader.width * 0.25
          duration: 280
          easing.type: Easing.OutCubic
        }
        onFinished: {
          if (pageLoader.outgoing) pageLoader.outgoing.source = ''
          pageLoader.outgoing = null
        }
      }
      Connections {
        target: mobile
        function onPageChanged() { pageLoader.showPage() }
      }
    }
    Rectangle {
      objectName: 'mobileBottomNav'
      Layout.fillWidth: true
      Layout.preferredHeight: visible ? Theme.bottomNavHeight : 0
      visible: root.isTab
      color: Theme.card
      Rectangle {
        width: parent.width
        height: 1
        color: Theme.border
      }
      Row {
        anchors.fill: parent
        Repeater {
          model: [{
              "key": 'home',
              "label": '找电站',
              "icon": 'house'
            }, {
              "key": 'orders',
              "label": '订单',
              "icon": 'list'
            }, {
              "key": 'profile',
              "label": '我的',
              "icon": 'user'
            }]
          delegate: Button {
            id: tabButton
            required property var modelData
            width: root.width / 3
            height: Theme.bottomNavHeight
            objectName: 'tab_' + modelData.key
            padding: 0
            Accessible.name: modelData.label
            enabled: !mobile.busy
            highlighted: mobile.tab === modelData.key
            Accessible.role: Accessible.PageTab
            Accessible.selected: highlighted
            onClicked: mobile.selectTab(modelData.key)
            background: Rectangle {
              color: tabButton.down || tabButton.visualFocus ? Theme.primaryLight : 'transparent'
            }
            contentItem: Item {
              Column {
                anchors.centerIn: parent
                width: parent.width
                opacity: tabButton.enabled ? 1 : 0.5
                spacing: Theme.microSpace
                Rectangle {
                  anchors.horizontalCenter: parent.horizontalCenter
                  width: 64
                  height: 32
                  radius: 16
                  color: tabButton.highlighted ? Theme.accent : 'transparent'
                  AppIcon {
                    anchors.centerIn: parent
                    name: tabButton.modelData.icon
                    color: tabButton.highlighted ? Theme.surfaceDark : Theme.ink
                    opacity: tabButton.highlighted ? 1 : 0.65
                  }
                }
                AppText {
                  width: parent.width
                  text: tabButton.modelData.label
                  font.pixelSize: Theme.labelSize
                  font.weight: tabButton.highlighted ? Font.DemiBold : Font.Normal
                  color: tabButton.highlighted ? Theme.primaryText : Theme.muted
                  horizontalAlignment: Text.AlignHCenter
                }
              }
            }
          }
        }
      }
    }
  }
  Rectangle {
    id: errorBanner
    objectName: 'errorBanner'
    property string message: ''
    z: 20
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.margins: Theme.pagePadding
    radius: Theme.cardRadius
    y: mobile.error.length > 0 ? Theme.space : -height
    opacity: mobile.error.length > 0 ? 1 : 0
    Behavior on y { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
    Behavior on opacity { NumberAnimation { duration: 160 } }
    height: Math.max(64, errorText.implicitHeight + 32)
    visible: opacity > 0
    color: Theme.dangerLight
    RowLayout {
      anchors.fill: parent
      anchors.leftMargin: Theme.pagePadding
      anchors.rightMargin: Theme.pagePadding
      spacing: Theme.space
      AppText {
        id: errorText
        Layout.fillWidth: true
        text: errorBanner.message
        wrapMode: Text.WordWrap
        color: Theme.danger
      }
      IconButton {
        label: '关闭错误提示'
        iconName: 'x'
        onClicked: mobile.clearError()
      }
    }
  }
  Rectangle {
    id: toast
    objectName: 'toastPopup'
    anchors.centerIn: parent
    width: Math.min(root.width - Theme.pagePadding * 2, 360)
    height: toastText.implicitHeight + Theme.cardPadding * 2
    radius: Theme.cardRadius
    color: Theme.toast
    z: 10
    visible: opacity > 0
    opacity: 0
    property string message: ''
    function open() { opacity = 1 }
    function close() { opacity = 0 }
    Behavior on opacity { NumberAnimation { duration: 160 } }
    AppText {
      id: toastText
      anchors.centerIn: parent
      width: parent.width - Theme.cardPadding * 2
      text: toast.message
      color: 'white'
      wrapMode: Text.WordWrap
      horizontalAlignment: Text.AlignHCenter
    }
    Timer {
      id: toastTimer
      interval: 2400
      onTriggered: toast.close()
    }
  }
  Popup {
    id: unfinished
    objectName: 'unfinishedOrderPopup'
    anchors.centerIn: parent
    width: root.width - Theme.pagePadding * 2
    padding: Theme.cardPadding
    modal: true
    closePolicy: Popup.NoAutoClose
    property string message: ''
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
        text: '订单未完成'
        font.pixelSize: Theme.titleSize
        font.weight: Font.DemiBold
        wrapMode: Text.WordWrap
      }
      AppText {
        width: parent.width
        text: unfinished.message
        wrapMode: Text.WordWrap
        color: Theme.muted
        lineHeight: 1.5
      }
      ActionButton {
        objectName: 'unfinishedOrderContinue'
        width: parent.width
        text: '前往结算'
        onClicked: unfinished.close()
      }
    }
  }
  Connections {
    target: mobile
    function onErrorChanged() {
      if (mobile.error.length > 0) {
        errorBanner.message = mobile.error
        toast.close()
      }
    }
    function onNotification(message) {
      if (mobile.nativePlatform || mobile.error.length > 0) return
      toast.message = message
      toast.open()
      toastTimer.restart()
    }
    function onUnfinishedOrder(message) {
      toast.close()
      unfinished.message = message
      unfinished.open()
    }
  }
}
