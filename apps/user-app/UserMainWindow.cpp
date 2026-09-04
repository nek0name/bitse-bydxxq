#include "UserMainWindow.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
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
    layout->addWidget(valueLabel, 0, Qt::AlignLeft);
    layout->addWidget(descriptionLabel, 0, Qt::AlignLeft);
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

QWidget *createFindStationPage() {
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
    stationLayout->addWidget(createStationCard(
      "中关村充电站", "1.2 km", "空闲 8 / 12", "快充/慢充 · ¥1.20/度"));
    stationLayout->addWidget(createStationCard(
      "学院路充电站", "2.8 km", "空闲 4 / 10", "快充 · ¥1.15/度"));
    stationLayout->addWidget(createStationCard(
      "五道口充电站", "3.5 km", "空闲 6 / 16", "快充/慢充 · ¥1.30/度"));
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

QWidget *createOrdersPage() {
    auto *page = new QWidget;
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
    const auto populateOrders = [orders](int category) {
        orders->clear();
        if (category == 0 || category == 2) {
            orders->addItem("中关村充电站 · A01\n已完成 · 32.6 kWh · ¥39.12");
        }
        if (category == 0 || category == 3) {
            orders->addItem("学院路充电站 · B03\n已取消 · 未产生费用");
        }
        if (orders->count() == 0) {
            orders->addItem("当前分类暂无订单");
        }
    };
    QObject::connect(tabs, &QTabBar::currentChanged, orders, populateOrders);
    populateOrders(0);

    layout->addWidget(createHeading("我的订单"));
    layout->addWidget(tabs);
    layout->addWidget(orders, 1);
    return page;
}

QWidget *createProfilePage() {
    auto *content = new QWidget;
    content->setObjectName("page");
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(kMargin, kMargin, kMargin, kMargin);
    layout->setSpacing(kSpacing);
    layout->addWidget(createHeading("个人中心"));

    auto *account = new QGroupBox("账户信息");
    account->setObjectName("sectionCard");
    auto *accountLayout = new QGridLayout(account);
    accountLayout->addWidget(new QLabel("昵称"), 0, 0);
    accountLayout->addWidget(new QLabel("体验用户"), 0, 1);
    accountLayout->addWidget(new QLabel("手机号"), 1, 0);
    accountLayout->addWidget(new QLabel("138****0000"), 1, 1);
    accountLayout->addWidget(new QLabel("账户状态"), 2, 0);
    accountLayout->addWidget(new QLabel("正常"), 2, 1);
    accountLayout->setColumnStretch(1, 1);

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
    walletLayout->addWidget(new QLabel("余额：¥ 0.00"), 1);
    walletLayout->addWidget(new QPushButton("充值"));

    auto *actions = new QGroupBox("常用功能");
    actions->setObjectName("sectionCard");
    auto *actionLayout = new QGridLayout(actions);
    actionLayout->addWidget(new QPushButton("编辑资料"), 0, 0);
    actionLayout->addWidget(new QPushButton("更换头像"), 0, 1);
    actionLayout->addWidget(new QPushButton("历史订单"), 1, 0);
    actionLayout->addWidget(new QPushButton("充值记录"), 1, 1);
    layout->addWidget(account);
    layout->addLayout(summary);
    layout->addWidget(wallet);
    layout->addWidget(actions);
    layout->addWidget(new QPushButton("退出登录"));
    layout->addStretch(1);

    auto *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(content);
    return scrollArea;
}
} // namespace

UserMainWindow::UserMainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("充电用户端");
    resize(420, 720);
    setMinimumSize(320, 500);

    auto *central = new QWidget;
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    pages_ = new QStackedWidget;
    pages_->addWidget(createFindStationPage());
    pages_->addWidget(createOrdersPage());
    pages_->addWidget(createProfilePage());

    auto *navigation = new QFrame;
    navigation->setObjectName("bottomNav");
    auto *navigationLayout = new QHBoxLayout(navigation);
    navigationLayout->setContentsMargins(6, 6, 6, 6);
    navigationLayout->setSpacing(4);
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
