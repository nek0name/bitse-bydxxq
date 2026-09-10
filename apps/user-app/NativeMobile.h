#pragma once
#include <QColor>
#include <QObject>

class NativeMobile final : public QObject {
  Q_OBJECT
  Q_PROPERTY(qreal keyboardInset READ keyboardInset NOTIFY keyboardInsetChanged)
public:
  explicit NativeMobile(QObject *parent = nullptr);
  qreal keyboardInset() const { return m_keyboardInset; }
  void configureKeyboard();
  void setKeyboardInset(qreal inset);
  void updateBars(const QColor &color, bool dark);
  void showToast(const QString &message);
  void requestLocation();
  void pickLocation(double latitude, double longitude, bool known);
signals:
  void keyboardInsetChanged();
  void locationReady(double latitude, double longitude, const QString &name);
  void locationFailed(const QString &message);

private:
  qreal m_keyboardInset = 0;
};
