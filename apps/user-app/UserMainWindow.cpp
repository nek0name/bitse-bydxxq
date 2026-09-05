#include "UserMainWindow.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStackedWidget>
#include <QStringList>
#include <QTabBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <functional>

namespace {
constexpr int kMargin = 12;
constexpr int kSpacing = 8;

struct StationRow {
    QString name;
    QString address;
    QString region;
    int idle = 0;
    int total = 0;
    double price = 0;
};

QSqlDatabase openDatabase() {
    QString path = qEnvironmentVariable("CHARGING_DB_PATH");
    if (path.isEmpty()) {
        QDir dir(QCoreApplication::applicationDirPath());
        const QStringList candidates = {
          dir.filePath("../../../../database/charging_platform.db"),
          dir.filePath("../../../database/charging_platform.db"),
          QDir::current().filePath("database/charging_platform.db")};
        for (const auto &candidate : candidates) {
            if (QFileInfo::exists(candidate)) {
                path = QFileInfo(candidate).canonicalFilePath();
                break;
            }
        }
    }
    if (path.isEmpty()) return {};
    const QString connectionName = "user-app-readonly";
    if (QSqlDatabase::contains(connectionName))
        return QSqlDatabase::database(connectionName);
    auto db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(path);
    if (!db.open()) return {};
    QSqlQuery pragma(db);
    pragma.exec("PRAGMA foreign_keys = ON");
    pragma.exec("PRAGMA busy_timeout = 5000");
    return db;
}

qint64 currentUserId() {
    return qEnvironmentVariable("CHARGING_USER_ID", "1").toLongLong();
}

QList<StationRow> loadStations(const QSqlDatabase &db) {
    QList<StationRow> rows;
    if (!db.isOpen()) return rows;
    QSqlQuery query(db);
    query.prepare(
      "SELECT v.name, v.address, COALESCE(v.region_code, ''), "
      "COALESCE(v.idle_charger_count, 0), COALESCE(v.charger_count, 0), "
      "COALESCE(t.electricity_price + t.service_price, 0) "
      "FROM v_station_availability v LEFT JOIN tariffs t "
      "ON t.station_id = v.station_id AND t.charger_type = 'all' "
      "ORDER BY v.station_id");
    if (!query.exec()) return rows;
    while (query.next()) {
        rows.append({query.value(0).toString(), query.value(1).toString(),
                     query.value(2).toString(), query.value(3).toInt(),
                     query.value(4).toInt(), query.value(5).toDouble()});
    }
    return rows;
}

class ClickableFrame final : public QFrame {
  public:
    explicit ClickableFrame(QWidget *parent = nullptr) : QFrame(parent) {
        setFrameShape(QFrame::StyledPanel);
        setCursor(Qt::PointingHandCursor);
    }

    void setClickedHandler(std::function<void()> handler) {
        clickedHandler_ = std::move(handler);
    }

  protected:
    void enterEvent(QEnterEvent *event) override {
        setFrameShadow(QFrame::Raised);
        QFrame::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        setFrameShadow(QFrame::Plain);
        QFrame::leaveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && rect().contains(event->pos())
            && clickedHandler_) {
            clickedHandler_();
        }
        QFrame::mouseReleaseEvent(event);
    }

  private:
    std::function<void()> clickedHandler_;
};

QLabel *createHeading(const QString &text) {
    auto *label = new QLabel(text);
    label->setObjectName("heading");
    QFont font = label->font();
    font.setPointSize(font.pointSize() + 3);
    font.setBold(true);
    label->setFont(font);
    return label;
}

QFrame *createSummaryItem(const QString &value, const QString &label) {
    auto *item = new QFrame;
    auto *layout = new QVBoxLayout(item);
    layout->setContentsMargins(2, 4, 2, 4);
    layout->setSpacing(1);
    auto *valueLabel = new QLabel(value);
    QFont valueFont = valueLabel->font();
    valueFont.setBold(true);
    valueFont.setPointSize(valueFont.pointSize() + 3);
    valueLabel->setFont(valueFont);
    auto *descriptionLabel = new QLabel(label);
    QFont descriptionFont = descriptionLabel->font();
    descriptionFont.setPointSize(qMax(8, descriptionFont.pointSize() - 1));
    descriptionLabel->setFont(descriptionFont);
    layout->addWidget(valueLabel, 0, Qt::AlignCenter);
    layout->addWidget(descriptionLabel, 0, Qt::AlignCenter);
    return item;
}

void showStationDetails(QWidget *parent, const QString &name,
                        const QString &availability, const QString &detail) {
    QDialog dialog(parent);
    dialog.setWindowTitle(name);
    dialog.setMinimumWidth(320);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(createHeading(name));
    layout->addWidget(new QLabel(availability));
    layout->addWidget(new QLabel(detail));
    layout->addWidget(new QLabel("可用设备"));
    layout->addWidget(new QLabel("A01 · 直流快充 · 120 kW · 空闲"));
    layout->addWidget(new QLabel("A02 · 交流慢充 · 7 kW · 空闲"));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog,
                     &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();
}

ClickableFrame *createStationCard(const QString &name, const QString &distance,
                                  const QString &availability,
                                  const QString &detail) {
    auto *card = new ClickableFrame;
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(4);

    auto *titleRow = new QHBoxLayout;
    auto *nameLabel = new QLabel(name);
    QFont nameFont = nameLabel->font();
    nameFont.setBold(true);
    nameLabel->setFont(nameFont);
    titleRow->addWidget(nameLabel, 1);
    titleRow->addWidget(new QLabel(distance));
    auto *detailLabel = new QLabel(detail);
    QFont detailFont = detailLabel->font();
    detailFont.setPointSize(qMax(8, detailFont.pointSize() - 1));
    detailLabel->setFont(detailFont);

    const QStringList parts = availability.split(' ');
    const int available = parts.value(1).toInt();
    const int total = parts.value(3).toInt();
    auto *capacityRow = new QHBoxLayout;
    auto *capacity = new QProgressBar;
    capacity->setRange(0, total);
    capacity->setValue(available);
    capacity->setTextVisible(false);
    capacity->setMaximumHeight(10);
    capacityRow->addWidget(capacity, 1);
    auto *capacityText = new QLabel(
      QString("%1/%2 空闲").arg(available).arg(total));
    QFont capacityFont = capacityText->font();
    capacityFont.setPointSize(qMax(8, capacityFont.pointSize() - 1));
    capacityText->setFont(capacityFont);
    capacityRow->addWidget(capacityText);
    auto *navigateButton = new QToolButton;
    navigateButton->setText("导航");
    navigateButton->setIcon(QIcon(":/icons/navigation.svg"));
    navigateButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    navigateButton->setAutoRaise(true);
    navigateButton->setToolTip("导航到" + name);
    QObject::connect(navigateButton, &QToolButton::clicked, card, [name] {
        const QString
          target = QString::fromLatin1(
                     "https://apis.map.qq.com/uri/v1/search?keyword=%1&region="
                     "%2&referer=charging-platform")
                     .arg(QString::fromUtf8(QUrl::toPercentEncoding(name)),
                          QString::fromUtf8(QUrl::toPercentEncoding("北京市")));
        QDesktopServices::openUrl(QUrl(target));
    });
    capacityRow->addWidget(navigateButton);

    layout->addLayout(titleRow);
    layout->addWidget(detailLabel);
    layout->addLayout(capacityRow);
    card->setClickedHandler([card, name, availability, detail] {
        showStationDetails(card, name, availability, detail);
    });
    return card;
}

QToolButton *createQuickAction(const QString &text, const QString &iconPath) {
    auto *button = new QToolButton;
    button->setText(text);
    button->setIcon(QIcon(iconPath));
    button->setIconSize(QSize(22, 22));
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setProperty("quickAction", true);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return button;
}

QWidget *createFindStationPage(const QList<StationRow> &stations) {
    auto *page = new QWidget;
    page->setObjectName("page");
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(kMargin, kMargin, kMargin, kMargin);
    layout->setSpacing(kSpacing);

    auto *headingRow = new QHBoxLayout;
    headingRow->addWidget(createHeading("你好，准备充电吗？"));
    auto *location = new QPushButton("北京市海淀区");
    location->setIcon(QIcon(":/icons/chevron-down.svg"));
    location->setLayoutDirection(Qt::RightToLeft);
    location->setFlat(true);
    location->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    auto *locationMenu = new QMenu(location);
    locationMenu->addActions({new QAction("北京市海淀区", locationMenu),
                              new QAction("北京市朝阳区", locationMenu),
                              new QAction("北京市东城区", locationMenu)});
    QObject::connect(
      location, &QPushButton::clicked, location, [location, locationMenu] {
          locationMenu->popup(
            location->mapToGlobal(QPoint(0, location->height())));
      });
    QObject::connect(locationMenu, &QMenu::triggered, location,
                     [location](QAction *action) {
                         location->setText(action->text());
                     });
    headingRow->addWidget(location);
    headingRow->addStretch(1);
    auto *refreshButton = new QToolButton;
    refreshButton->setIcon(QIcon(":/icons/refresh-cw.svg"));
    refreshButton->setIconSize(QSize(16, 16));
    refreshButton->setToolTip("刷新当前位置和电站状态");
    const int refreshSide = location->sizeHint().height();
    refreshButton->setFixedSize(refreshSide, refreshSide);
    headingRow->addWidget(refreshButton);

    auto *searchRow = new QHBoxLayout;
    auto *locationInput = new QLineEdit;
    locationInput->setClearButtonEnabled(true);
    locationInput->setPlaceholderText("搜索地点、充电站");
    locationInput->setObjectName("searchInput");
    searchRow->addWidget(locationInput, 1);
    auto *searchButton = new QToolButton;
    searchButton->setIcon(QIcon(":/icons/search.svg"));
    searchButton->setIconSize(QSize(16, 16));
    searchButton->setToolTip("搜索");
    searchButton->setProperty("iconButton", true);
    const int searchSide = locationInput->sizeHint().height();
    searchButton->setFixedSize(searchSide, searchSide);
    searchRow->addWidget(searchButton);

    auto *quickActions = new QGridLayout;
    quickActions->setSpacing(6);
    quickActions->addWidget(
      createQuickAction("离我最近", ":/icons/navigation.svg"), 0, 0);
    quickActions->addWidget(createQuickAction("快速充电", ":/icons/zap.svg"), 0,
                            1);
    quickActions->addWidget(createQuickAction("低价优选", ":/icons/search.svg"),
                            0, 2);
    quickActions->addWidget(createQuickAction("我的收藏", ":/icons/house.svg"),
                            0, 3);
    for (int column = 0; column < 4; ++column) {
        quickActions->setColumnStretch(column, 1);
    }

    auto *filterRow = new QHBoxLayout;
    auto *areaFilter = new QComboBox;
    areaFilter->addItems({"全部区域", "海淀区", "朝阳区", "东城区"});
    filterRow->addWidget(areaFilter, 1);
    auto *sortGroup = new QButtonGroup(page);
    const QStringList sortLabels{"距离", "空闲", "价格"};
    for (int index = 0; index < sortLabels.size(); ++index) {
        auto *sortButton = new QPushButton(sortLabels.at(index));
        sortButton->setCheckable(true);
        sortButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sortGroup->addButton(sortButton, index);
        filterRow->addWidget(sortButton, 1);
    }
    sortGroup->button(0)->setChecked(true);

    auto *stationContent = new QWidget;
    auto *stationLayout = new QVBoxLayout(stationContent);
    stationLayout->setContentsMargins(0, 0, 0, 0);
    stationLayout->setSpacing(6);
    if (stations.isEmpty()) {
        stationLayout->addWidget(new QLabel("暂无电站数据，请检查数据库连接"));
    } else {
        for (const auto &station : stations) {
            const QString availability = QString("空闲 %1 / %2")
                                           .arg(station.idle)
                                           .arg(station.total);
            const QString detail = QString("%1 · ¥%2/度")
                                     .arg(station.region.isEmpty()
                                            ? station.address
                                            : station.region)
                                     .arg(station.price, 0, 'f', 2);
            stationLayout->addWidget(
              createStationCard(station.name, "", availability, detail));
        }
    }
    stationLayout->addStretch(1);
    auto *stationScroll = new QScrollArea;
    stationScroll->setWidgetResizable(true);
    stationScroll->setFrameShape(QFrame::NoFrame);
    stationScroll->setWidget(stationContent);

    layout->addLayout(headingRow);
    layout->addLayout(searchRow);
    layout->addLayout(quickActions);
    layout->addWidget(createHeading("推荐电站"));
    layout->addLayout(filterRow);
    layout->addWidget(stationScroll, 1);
    return page;
}

QWidget *createOrdersPage(const QSqlDatabase &db) {
    auto *page = new QWidget;
    page->setObjectName("ordersPage");
    page->setStyleSheet(R"QSS(
        QWidget#ordersPage { background: #f7fafc; }
        QTabBar { background: transparent; }
        QTabBar::tab {
            color: #84929d;
            min-height: 38px;
            padding: 0 14px;
            border: none;
        }
        QTabBar::tab:selected {
            color: #2f7fae;
            font-weight: 600;
            border-bottom: 2px solid #2f7fae;
        }
        QListWidget {
            background: transparent;
            border: none;
            outline: none;
        }
        QFrame#orderCard {
            background: #ffffff;
            border: 1px solid #e3ebf0;
            border-radius: 12px;
        }
        QLabel#orderStation { color: #17212b; font-size: 15px; font-weight: 600; }
        QLabel#orderStatus { color: #2f7fae; font-size: 13px; font-weight: 600; }
        QLabel#orderMeta { color: #526473; font-size: 13px; }
        QLabel#orderAmount { color: #17212b; font-size: 18px; font-weight: 700; }
        QLabel#orderTime { color: #8b98a3; font-size: 12px; }
        QPushButton#orderAction {
            min-height: 30px;
            padding: 0 14px;
            border: 1px solid #b8cbd5;
            border-radius: 15px;
            background: #ffffff;
            color: #526473;
        }
        QPushButton#orderAction:hover { background: #f0f7fa; border-color: #78abc1; }
    )QSS");
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(kMargin, kMargin, kMargin, kMargin);
    layout->setSpacing(kSpacing);

    auto *tabs = new QTabBar;
    tabs->addTab("全部");
    tabs->addTab("进行中");
    tabs->addTab("已完成");
    tabs->addTab("已取消");
    tabs->setExpanding(true);

    auto *orders = new QListWidget;
    orders->setSelectionMode(QAbstractItemView::NoSelection);
    orders->setSpacing(8);
    const auto populateOrders = [orders, db](int category) {
        orders->clear();
        if (!db.isOpen()) {
            orders->addItem("数据库未连接");
            return;
        }
        QSqlQuery query(db);
        QString condition;
        if (category == 1) condition = " AND o.status = 'pending_payment'";
        if (category == 2) condition = " AND o.status = 'paid'";
        if (category == 3) condition = " AND o.status = 'cancelled'";
        query.exec("SELECT s.name, c.charger_code, o.status, o.energy_kwh, "
                   "o.paid_amount, o.created_at "
                   "FROM orders o JOIN stations s ON s.id=o.station_id JOIN "
                   "chargers c ON c.id=o.charger_id "
                   "WHERE o.user_id="
                   + QString::number(currentUserId()) + condition
                   + " ORDER BY o.created_at DESC LIMIT 30");
        while (query.next()) {
            const QString status = query.value(2).toString();
            const QMap<QString, QString> statusLabels{
              {"pending_payment", "待支付"},
              {"paid", "已完成"},
              {"payment_failed", "支付失败"},
              {"cancelled", "已取消"},
              {"refunded", "已退款"}};
            const QString displayStatus = statusLabels.value(status, status);
            const QString createdAt = query.value(5).toString();
            const QDateTime orderTime = QDateTime::fromString(createdAt,
                                                              Qt::ISODate);
            const QString displayTime = orderTime.isValid()
                                        ? orderTime.toString("yyyy.MM.dd HH:mm")
                                        : createdAt;

            auto *item = new QListWidgetItem;
            auto *card = new QFrame;
            card->setObjectName("orderCard");
            auto *cardLayout = new QVBoxLayout(card);
            cardLayout->setContentsMargins(14, 12, 14, 12);
            cardLayout->setSpacing(7);

            auto *header = new QHBoxLayout;
            auto *stationLabel = new QLabel(query.value(0).toString());
            stationLabel->setObjectName("orderStation");
            header->addWidget(stationLabel, 1);
            auto *statusLabel = new QLabel(displayStatus);
            statusLabel->setObjectName("orderStatus");
            header->addWidget(statusLabel, 0, Qt::AlignTop);

            auto *summary = new QHBoxLayout;
            auto *metaLabel = new QLabel(
              QString("%1 · %2 kWh")
                .arg(query.value(1).toString(), query.value(3).toString()));
            metaLabel->setObjectName("orderMeta");
            summary->addWidget(metaLabel, 1);
            auto *amountLabel = new QLabel(
              QString("¥%1").arg(query.value(4).toDouble(), 0, 'f', 2));
            amountLabel->setObjectName("orderAmount");
            summary->addWidget(amountLabel, 0, Qt::AlignVCenter);

            auto *footer = new QHBoxLayout;
            auto *timeLabel = new QLabel("下单时间  " + displayTime);
            timeLabel->setObjectName("orderTime");
            footer->addWidget(timeLabel, 1);
            auto *detailsButton = new QPushButton("查看详情");
            detailsButton->setObjectName("orderAction");
            footer->addWidget(detailsButton);

            cardLayout->addLayout(header);
            cardLayout->addLayout(summary);
            cardLayout->addLayout(footer);
            orders->addItem(item);
            orders->setItemWidget(item, card);
            item->setSizeHint(card->sizeHint());
        }
        if (orders->count() == 0) {
            auto *emptyItem = new QListWidgetItem("当前分类暂无订单");
            emptyItem->setFlags(Qt::NoItemFlags);
            orders->addItem(emptyItem);
        }
    };
    QObject::connect(tabs, &QTabBar::currentChanged, orders, populateOrders);
    populateOrders(0);

    layout->addWidget(createHeading("我的订单"));
    layout->addWidget(tabs);
    layout->addWidget(orders, 1);
    return page;
}

QWidget *createProfilePage(const QSqlDatabase &db) {
    auto *content = new QWidget;
    content->setObjectName("page");
    content->setStyleSheet(R"QSS(
        QWidget#page { background: #f7fafc; }
        QLabel#heading { color: #17212b; }
        QGroupBox#sectionCard {
            background: rgba(255, 255, 255, 235);
            border: 1px solid #e3ebf0;
            border-radius: 12px;
            margin-top: 8px;
            padding: 16px 12px 12px 12px;
        }
        QGroupBox#sectionCard::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
            color: #526473;
            font-weight: 600;
        }
        QPushButton {
            min-height: 44px;
            border-radius: 10px;
            border: 1px solid #d8e3ea;
            background: #ffffff;
            color: #2f7fae;
            font-size: 14px;
        }
        QPushButton:pressed { background: #edf5f9; }
    )QSS");
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(kMargin, kMargin, kMargin, kMargin);
    layout->setSpacing(14);
    QString nickname = "用户";
    QString phone = "未登录";
    QString status = "未知";
    double balance = 0;
    if (db.isOpen()) {
        QSqlQuery query(db);
        query.prepare("SELECT nickname, phone, status, wallet_balance FROM "
                      "users WHERE id = :id");
        query.bindValue(":id", currentUserId());
        query.exec();
        if (query.next()) {
            nickname = query.value(0).toString();
            phone = query.value(1).toString();
            status = query.value(2).toString();
            balance = query.value(3).toDouble();
        }
    }
    auto *account = new QGroupBox;
    account->setObjectName("sectionCard");
    account->setMinimumHeight(96);
    auto *accountLayout = new QHBoxLayout(account);
    accountLayout->setContentsMargins(54, 8, 12, 8);
    accountLayout->setSpacing(64);
    auto *avatar = new QLabel("头像");
    avatar->setFixedSize(58, 58);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet("background: #dceff5; color: #4e91ad; border-radius: "
                          "29px; font-size: 12px;");
    auto *nicknameLabel = new QLabel(nickname);
    nicknameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QFont nicknameFont = nicknameLabel->font();
    nicknameFont.setPointSize(16);
    nicknameFont.setBold(true);
    nicknameLabel->setFont(nicknameFont);
    auto maskedPhone = phone;
    if (maskedPhone.size() == 11)
        for (int index = 3; index < 7; ++index) maskedPhone[index] = '*';
    auto *phoneLabel = new QLabel(maskedPhone);
    phoneLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    phoneLabel->setStyleSheet("color: #7b8994; font-size: 14px;");
    auto *profileInfo = new QVBoxLayout;
    profileInfo->setSpacing(4);
    profileInfo->addWidget(nicknameLabel);
    profileInfo->addWidget(phoneLabel);
    accountLayout->addWidget(avatar, 0, Qt::AlignLeft | Qt::AlignVCenter);
    accountLayout->addLayout(profileInfo, 1);

    auto *summary = new QGridLayout;
    summary->setSpacing(6);
    summary->addWidget(createSummaryItem("0.0 kWh", "本月电量"), 0, 0);
    summary->addWidget(createSummaryItem("¥ 0.00", "本月消费"), 0, 1);
    summary->addWidget(createSummaryItem("0", "充电次数"), 0, 2);
    summary->addWidget(createSummaryItem("--", "最近充电"), 0, 3);
    for (int column = 0; column < 4; ++column) {
        summary->setColumnStretch(column, 1);
    }

    auto *wallet = new QGroupBox("我的钱包");
    wallet->setObjectName("sectionCard");
    auto *walletLayout = new QHBoxLayout(wallet);
    auto *balanceLabel = new QLabel(QString("¥ %1").arg(balance, 0, 'f', 2));
    QFont balanceFont = balanceLabel->font();
    balanceFont.setPointSize(20);
    balanceFont.setBold(true);
    balanceLabel->setFont(balanceFont);
    walletLayout->addWidget(balanceLabel, 1);
    auto *rechargeButton = new QPushButton("充值");
    rechargeButton->setMinimumWidth(86);
    walletLayout->addWidget(rechargeButton);

    layout->addWidget(account);
    layout->addSpacing(12);
    layout->addLayout(summary);
    layout->addSpacing(12);
    layout->addWidget(wallet);
    layout->addStretch(1);
    auto *logoutButton = new QPushButton("退出登录");
    logoutButton->setStyleSheet(
      "color: #7b8994; background: transparent; border: none;");
    layout->addWidget(logoutButton);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(content);
    return scrollArea;
}
} // namespace

UserMainWindow::UserMainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("智充出行");
    resize(420, 720);
    setMinimumSize(320, 500);

    auto *central = new QWidget;
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    pages_ = new QStackedWidget;
    const auto db = openDatabase();
    pages_->addWidget(createFindStationPage(loadStations(db)));
    pages_->addWidget(createOrdersPage(db));
    auto *profilePage = createProfilePage(db);
    pages_->addWidget(profilePage);
    for (auto *button : profilePage->findChildren<QPushButton *>()) {
        if (button->text() == "退出登录") {
            connect(button, &QPushButton::clicked, this, [this] {
                if (logoutHandler_) logoutHandler_();
            });
        }
    }

    auto *navigation = new QFrame;
    navigation->setObjectName("bottomNav");
    navigation->setStyleSheet(R"QSS(
        QFrame#bottomNav {
            background: #ffffff;
            border-top: 1px solid #e7edf1;
        }
        QToolButton[nav] {
            min-height: 46px;
            border: none;
            border-radius: 8px;
            color: #8b98a3;
            font-size: 12px;
            padding: 1px 0 0 0;
        }
        QToolButton[nav]:checked {
            color: #2f7fae;
            font-weight: 600;
            background: #f0f7fa;
        }
        QToolButton[nav]:pressed { background: #e7f1f5; }
    )QSS");
    auto *navigationLayout = new QHBoxLayout(navigation);
    navigationLayout->setContentsMargins(4, 3, 4, 3);
    navigationLayout->setSpacing(2);
    auto *buttonGroup = new QButtonGroup(this);
    const QStringList labels{"首页", "订单", "我的"};
    const QStringList icons{":/icons/house.svg", ":/icons/list.svg",
                            ":/icons/user.svg"};
    for (int index = 0; index < labels.size(); ++index) {
        auto *button = new QToolButton;
        button->setText(labels.at(index));
        button->setIcon(QIcon(icons.at(index)));
        button->setIconSize(QSize(22, 22));
        button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setProperty("nav", true);
        button->setCheckable(true);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        navigationLayout->addWidget(button);
        buttonGroup->addButton(button, index);
    }
    buttonGroup->button(0)->setChecked(true);
    connect(buttonGroup, &QButtonGroup::idClicked, pages_,
            &QStackedWidget::setCurrentIndex);
    rootLayout->addWidget(pages_, 1);
    rootLayout->addWidget(navigation);
    setCentralWidget(central);
}
