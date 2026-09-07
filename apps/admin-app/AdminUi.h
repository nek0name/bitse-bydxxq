#pragma once

#include "DataTable.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QVariantList>

class QChart;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QTableWidget;

namespace adminui {
void applyTheme();
void showSettings(QWidget *parent);
void styleChart(QChart *chart);
QString money(const QJsonValue &value);
QString number(const QJsonValue &value, int decimals = 1);
QVariant numberCell(const QJsonValue &value, int decimals = 1);
QVariant moneyCell(const QJsonValue &value);
QString timeText(const QJsonValue &value);
QString duration(const QJsonValue &value);
QString state(const QString &value);
QLabel *heading(const QString &text);
QJsonObject selected(QTableWidget *tableWidget);
QStringList chargerHeaders();
QVariantList chargerColumns(const QJsonObject &o);
QStringList orderHeaders();
QVariantList orderColumns(const QJsonObject &o);
QPushButton *button(const QString &text, QHBoxLayout *row);
} // namespace adminui
