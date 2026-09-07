pragma Singleton
import QtQuick

QtObject {
  readonly property color paper: appearance.colors.paper
  readonly property color card: appearance.colors.card
  readonly property color ink: appearance.colors.ink
  readonly property color muted: appearance.colors.muted
  readonly property color primaryText: appearance.colors.primaryText
  readonly property color primaryForeground: appearance.colors.onPrimary
  readonly property color secondary: appearance.colors.secondary
  readonly property color secondaryLight: appearance.colors.secondaryLight
  readonly property color primary: appearance.colors.primary
  readonly property color primaryPressed: appearance.colors.primaryPressed
  readonly property color primaryLight: appearance.colors.primaryLight
  readonly property color primarySoftPressed: appearance.colors.primarySoftPressed
  readonly property color accent: appearance.colors.accent
  readonly property color border: appearance.colors.border
  readonly property color surfaceDark: appearance.colors.surfaceDark
  readonly property color surfaceDarkRaised: appearance.colors.surfaceDarkRaised
  readonly property color heroMuted: appearance.colors.heroMuted
  readonly property color amber: appearance.colors.amber
  readonly property color amberLight: appearance.colors.amberLight
  readonly property color amberPressed: appearance.colors.amberPressed
  readonly property color danger: appearance.colors.danger
  readonly property color dangerLight: appearance.colors.dangerLight
  readonly property color disabled: appearance.colors.disabled
  readonly property color disabledText: appearance.colors.disabledText
  readonly property color overlay: appearance.colors.overlay
  readonly property color toast: appearance.colors.toast

  readonly property int pagePadding: 16
  readonly property int cardPadding: 16
  readonly property int space: 8
  readonly property int sectionSpace: 24
  readonly property int controlGap: 12
  readonly property int microSpace: 4
  readonly property int iconSize: 24
  readonly property int touchSize: 48
  readonly property int topBarHeight: 64
  readonly property int bottomNavHeight: 80
  readonly property int cardRadius: 16
  readonly property int heroRadius: 24
  readonly property int bodySize: 14
  readonly property int bodyLargeSize: 16
  readonly property int labelSize: 12
  readonly property int titleSize: 22
  readonly property int headlineSize: 28
}
