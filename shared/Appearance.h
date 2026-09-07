#pragma once

#include <QColor>
#include <QDBusVariant>
#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QVariantMap>

class Appearance final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY changed)
  Q_PROPERTY(
    QColor primaryColor READ primaryColor WRITE setPrimaryColor NOTIFY changed)
  Q_PROPERTY(QColor secondaryColor READ secondaryColor WRITE setSecondaryColor
               NOTIFY changed)
  Q_PROPERTY(bool dark READ dark NOTIFY changed)
  Q_PROPERTY(QVariantMap colors READ colors NOTIFY changed)
  Q_PROPERTY(QStringList swatches READ swatches CONSTANT)

public:
  static Appearance *instance();
  QString mode() const;
  void setMode(const QString &mode);
  QColor primaryColor() const;
  void setPrimaryColor(const QColor &color);
  QColor secondaryColor() const;
  void setSecondaryColor(const QColor &color);
  bool dark() const;
  QVariantMap colors() const;
  QStringList swatches() const;
  Q_INVOKABLE QString iconSource(const QString &resource,
                                 const QColor &color) const;

signals:
  void changed();

private slots:
  void systemSettingChanged(const QString &group, const QString &key,
                            const QDBusVariant &value);

private:
  explicit Appearance(QObject *parent);
  void setSystemDark(bool dark);
  void save(const QString &key, const QVariant &value);
  QSettings settings_;
  QString mode_;
  QColor primaryColor_;
  QColor secondaryColor_;
  bool systemDark_ = false;
  bool systemSettingReceived_ = false;
};
