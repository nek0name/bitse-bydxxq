import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Flickable {
  id: screen
  objectName: 'loginPage'
  property bool compact: phone.activeFocus || Qt.inputMethod.visible || height < 480
  property real inputProgress: compact ? 1 : 0
  property real brandHeight: Math.min(240, height * 0.3)
  Behavior on brandHeight {
    NumberAnimation { duration: 300; easing.type: Easing.InOutCubic }
  }
  Behavior on inputProgress {
    NumberAnimation { duration: 300; easing.type: Easing.InOutCubic }
  }
  contentWidth: width
  contentHeight: Math.max(height, form.y + form.implicitHeight)
  clip: true
  boundsBehavior: Flickable.StopAtBounds
  Rectangle {
    width: parent.width
    height: screen.brandHeight
    color: Theme.primaryLight
  }
  Row {
    id: brand
    z: 1
    x: (screen.width - width) / 2 * (1 - screen.inputProgress) + 24 * screen.inputProgress
    y: (screen.brandHeight - height) / 2 * (1 - screen.inputProgress) + 24 * screen.inputProgress
    spacing: 14
    Image {
      source: 'qrc:/assets/brand.svg'
      width: 56 - 20 * screen.inputProgress
      height: width
      Accessible.ignored: true
    }
    AppText {
      anchors.verticalCenter: parent.verticalCenter
      text: '智充出行'
      font.pixelSize: 32 - 8 * screen.inputProgress
      font.weight: Font.Bold
    }
  }
  Rectangle {
    id: form
    y: (screen.brandHeight - 16) * (1 - screen.inputProgress)
    width: parent.width
    height: Math.max(implicitHeight, screen.height - y)
    implicitHeight: content.y + content.height + 32
    radius: 24 * (1 - screen.inputProgress)
    color: Theme.card
    Column {
      id: content
      x: 24
      y: 40 + 56 * screen.inputProgress
      width: parent.width - 48
      spacing: 24
      AppText {
        text: '手机号登录'
        font.pixelSize: 26
        font.weight: Font.Bold
      }
      AppField {
        id: phone
        objectName: 'phoneInput'
        width: parent.width
        height: 56
        placeholderText: '请输入手机号'
        leftPadding: 76
        background: Rectangle {
          color: Theme.paper
          radius: 14
          AppText {
            x: 16
            anchors.verticalCenter: parent.verticalCenter
            text: '+86'
            color: Theme.ink
            font.pixelSize: 18
          }
          Rectangle {
            x: 62
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: 24
            color: Theme.border
          }
        }
        maximumLength: 11
        inputMethodHints: Qt.ImhDigitsOnly
        validator: RegularExpressionValidator { regularExpression: /[0-9]{0,11}/ }
        onAccepted: { Qt.inputMethod.hide(); mobile.login(text) }
        Accessible.name: '手机号'
      }
      ActionButton {
        objectName: 'loginButton'
        width: parent.width
        implicitHeight: 56
        text: mobile.busy ? '正在登录…' : '登录 / 注册'
        enabled: !mobile.busy
        onClicked: { Qt.inputMethod.hide(); mobile.login(phone.text) }
      }
    }
  }
  Popup {
    id: registration
    objectName: 'registrationPopup'
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(screen.width - 32, 400)
    padding: 24
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    property string phoneNumber: ''
    background: Rectangle { color: Theme.card; radius: 24 }
    Overlay.modal: Rectangle { color: Theme.overlay }
    contentItem: Column {
      spacing: 20
      AppText {
        width: parent.width
        text: '注册新账号？'
        font.pixelSize: Theme.titleSize
        font.weight: Font.DemiBold
      }
      AppText {
        width: parent.width
        text: '手机号 ' + registration.phoneNumber + '\n是否注册并登录？'
        wrapMode: Text.Wrap
        color: Theme.muted
        lineHeight: 1.4
      }
      RowLayout {
        width: parent.width
        spacing: 12
        ActionButton {
          objectName: 'cancelRegistrationButton'
          Layout.fillWidth: true
          Layout.preferredWidth: 1
          text: '取消'
          variant: 'secondary'
          onClicked: registration.close()
        }
        ActionButton {
          objectName: 'confirmRegistrationButton'
          Layout.fillWidth: true
          Layout.preferredWidth: 1
          text: '注册并登录'
          enabled: !mobile.busy
          onClicked: {
            registration.close()
            mobile.confirmRegistration(registration.phoneNumber)
          }
        }
      }
    }
  }
  Connections {
    target: mobile
    function onRegistrationRequested(phoneNumber) {
      Qt.inputMethod.hide()
      registration.phoneNumber = phoneNumber
      registration.open()
    }
  }
}
