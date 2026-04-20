#include "DashboardWidget.h"
#include "DatabaseManager.h"
#include "AccountModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSqlQuery>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QPainter>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

// ─── 잔액 요약 카드 ──────────────────────────────────────────
static QGroupBox* makeSummaryCard(const QString& title, QLabel*& valLabel,
                                  const QString& color, QWidget* parent) {
    auto* box = new QGroupBox(parent);
    box->setMinimumWidth(200);
    box->setStyleSheet(QString(
        "QGroupBox { border-left: 4px solid %1; border-radius: 10px; "
        "background: white; padding: 16px 20px 20px 20px; margin-top: 0; }").arg(color));

    auto* titleLbl = new QLabel(title, box);
    titleLbl->setStyleSheet("color:#64748B; font-size:9pt; font-weight:600;");

    valLabel = new QLabel("₩0", box);
    valLabel->setStyleSheet(
        QString("color:%1; font-size:20pt; font-weight:700; letter-spacing:-0.5px;").arg(color));
    valLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* l = new QVBoxLayout(box);
    l->setSpacing(6);
    l->addWidget(titleLbl);
    l->addWidget(valLabel);
    return box;
}

// ─── 환율 카드 ─────────────────────────────────────────────
static QGroupBox* makeRateCard(const QString& code, const QString& name,
                                const QString& flag, QLabel*& valLabel,
                                QWidget* parent) {
    auto* box = new QGroupBox(parent);
    box->setStyleSheet(
        "QGroupBox { border: 1px solid #E2E8F0; border-radius: 10px; "
        "background: white; padding: 12px 16px 16px 16px; margin-top: 0; }");
    box->setMinimumWidth(150);

    auto* topRow = new QHBoxLayout;
    auto* flagLbl = new QLabel(flag + "  " + code, box);
    flagLbl->setStyleSheet("font-size:11pt; font-weight:700; color:#1A2233;");
    auto* nameLbl = new QLabel(name, box);
    nameLbl->setStyleSheet("font-size:8pt; color:#94A3B8;");
    topRow->addWidget(flagLbl);
    topRow->addStretch();

    valLabel = new QLabel("—", box);
    valLabel->setStyleSheet("font-size:14pt; font-weight:700; color:#1D4ED8; margin-top:4px;");

    auto* unitLbl = new QLabel(code == "JPY" ? "100엔 기준" : "1단위 기준", box);
    unitLbl->setStyleSheet("font-size:8pt; color:#94A3B8;");

    auto* l = new QVBoxLayout(box);
    l->setSpacing(2);
    l->addLayout(topRow);
    l->addWidget(nameLbl);
    l->addWidget(valLabel);
    l->addWidget(unitLbl);
    return box;
}

// ════════════════════════════════════════════════════════════
DashboardWidget::DashboardWidget(int userId, QWidget* parent)
    : QWidget(parent), m_userId(userId)
{
    setupUi();

    m_rateTimer = new QTimer(this);
    m_rateTimer->setInterval(5 * 60 * 1000); // 5분 자동 갱신
    connect(m_rateTimer, &QTimer::timeout, this, &DashboardWidget::fetchExchangeRates);
    m_rateTimer->start();

    refresh();
}

void DashboardWidget::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(14);

    // ── 1. 요약 카드 행 ─────────────────────────────────────
    auto* cardsRow = new QHBoxLayout;
    cardsRow->setSpacing(12);
    cardsRow->addWidget(makeSummaryCard("총 자산",         m_totalBalanceLabel, "#1D4ED8", this));
    cardsRow->addWidget(makeSummaryCard("이번 달 수입",    m_monthIncomeLabel,  "#10B981", this));
    cardsRow->addWidget(makeSummaryCard("이번 달 지출",    m_monthExpenseLabel, "#EF4444", this));
    root->addLayout(cardsRow);

    // ── 2. 환율 섹션 ────────────────────────────────────────
    auto* rateGroup = new QGroupBox("실시간 환율", this);
    auto* rateLayout = new QVBoxLayout(rateGroup);

    // 업데이트 시간 + 새로고침 버튼
    auto* rateHeader = new QHBoxLayout;
    m_rateUpdateLabel = new QLabel("업데이트 중...", this);
    m_rateUpdateLabel->setStyleSheet("color:#94A3B8; font-size:9pt;");
    auto* refreshBtn = new QPushButton("↻ 새로고침", this);
    refreshBtn->setFixedWidth(90);
    refreshBtn->setFixedHeight(28);
    refreshBtn->setStyleSheet("QPushButton { background:#E0ECFF; color:#1D4ED8; "
                               "border:none; border-radius:5px; font-size:9pt; font-weight:600; }"
                               "QPushButton:hover { background:#BFDBFE; }");
    connect(refreshBtn, &QPushButton::clicked, this, &DashboardWidget::fetchExchangeRates);
    rateHeader->addWidget(m_rateUpdateLabel);
    rateHeader->addStretch();
    rateHeader->addWidget(refreshBtn);
    rateLayout->addLayout(rateHeader);

    // 환율 카드 4개
    auto* rateCards = new QHBoxLayout;
    rateCards->setSpacing(10);
    rateCards->addWidget(makeRateCard("USD", "미국 달러",  "🇺🇸", m_usdLabel, this));
    rateCards->addWidget(makeRateCard("JPY", "일본 엔",    "🇯🇵", m_jpyLabel, this));
    rateCards->addWidget(makeRateCard("EUR", "유로",       "🇪🇺", m_eurLabel, this));
    rateCards->addWidget(makeRateCard("CNY", "중국 위안",  "🇨🇳", m_cnyLabel, this));
    rateLayout->addLayout(rateCards);
    root->addWidget(rateGroup);

    // ── 3. 차트 행 ──────────────────────────────────────────
    setupCharts();
    auto* chartRow = new QHBoxLayout;
    chartRow->setSpacing(12);
    chartRow->addWidget(m_barChartView, 3);
    chartRow->addWidget(m_pieChartView, 2);
    root->addLayout(chartRow, 1);
}

void DashboardWidget::setupCharts() {
    m_barChartView = new QChartView(this);
    m_pieChartView = new QChartView(this);
    auto* bar = static_cast<QChartView*>(m_barChartView);
    auto* pie = static_cast<QChartView*>(m_pieChartView);
    bar->setRenderHint(QPainter::Antialiasing);
    pie->setRenderHint(QPainter::Antialiasing);
    bar->setMinimumHeight(260);
    pie->setMinimumHeight(260);
    bar->setStyleSheet("background:white; border-radius:10px; border:1px solid #E2E8F0;");
    pie->setStyleSheet("background:white; border-radius:10px; border:1px solid #E2E8F0;");
}

void DashboardWidget::refresh() {
    updateSummaryCards();
    updateBarChart();
    updatePieChart();
    fetchExchangeRates();
}

void DashboardWidget::updateSummaryCards() {
    auto db = DatabaseManager::instance().database();
    QSqlQuery q(db);

    auto fmt = [](double v) { return AccountModel::formatKRW(v); };

    q.prepare("SELECT COALESCE(SUM(balance),0) FROM accounts WHERE user_id=:uid");
    q.bindValue(":uid", m_userId);
    if (q.exec() && q.next()) m_totalBalanceLabel->setText(fmt(q.value(0).toDouble()));

    q.prepare(R"(SELECT COALESCE(SUM(t.amount),0)
        FROM transactions t JOIN accounts a ON t.account_id=a.id
        WHERE a.user_id=:uid AND t.type='입금'
          AND strftime('%Y-%m',t.created_at)=strftime('%Y-%m','now','localtime'))");
    q.bindValue(":uid", m_userId);
    if (q.exec() && q.next()) m_monthIncomeLabel->setText(fmt(q.value(0).toDouble()));

    q.prepare(R"(SELECT COALESCE(SUM(t.amount),0)
        FROM transactions t JOIN accounts a ON t.account_id=a.id
        WHERE a.user_id=:uid AND t.type='출금'
          AND strftime('%Y-%m',t.created_at)=strftime('%Y-%m','now','localtime'))");
    q.bindValue(":uid", m_userId);
    if (q.exec() && q.next()) m_monthExpenseLabel->setText(fmt(q.value(0).toDouble()));
}

void DashboardWidget::updateBarChart() {
    auto db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT strftime('%m',t.created_at) as month,
               SUM(CASE WHEN t.type='입금'  THEN t.amount ELSE 0 END) as income,
               SUM(CASE WHEN t.type='출금' THEN t.amount ELSE 0 END) as expense
        FROM transactions t JOIN accounts a ON t.account_id=a.id
        WHERE a.user_id=:uid AND strftime('%Y',t.created_at)=strftime('%Y','now','localtime')
        GROUP BY month ORDER BY month)");
    q.bindValue(":uid", m_userId);

    auto* incomeSet  = new QBarSet("수입");
    auto* expenseSet = new QBarSet("지출");
    incomeSet->setColor(QColor("#10B981"));
    expenseSet->setColor(QColor("#EF4444"));
    QStringList months;

    if (q.exec()) {
        while (q.next()) {
            months << q.value(0).toString() + "월";
            *incomeSet  << q.value(1).toDouble();
            *expenseSet << q.value(2).toDouble();
        }
    }
    if (months.isEmpty()) { months << "—"; *incomeSet << 0; *expenseSet << 0; }

    auto* series = new QBarSeries;
    series->append(incomeSet);
    series->append(expenseSet);

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setTitle("월별 수입 / 지출");
    chart->setTitleFont(QFont("맑은 고딕", 10, QFont::Bold));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false);
    chart->legend()->setAlignment(Qt::AlignTop);

    auto* axisX = new QBarCategoryAxis; axisX->append(months);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    auto* axisY = new QValueAxis;
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    static_cast<QChartView*>(m_barChartView)->setChart(chart);
}

void DashboardWidget::updatePieChart() {
    auto db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT t.category, SUM(t.amount)
        FROM transactions t JOIN accounts a ON t.account_id=a.id
        WHERE a.user_id=:uid AND t.type='출금'
          AND strftime('%Y-%m',t.created_at)=strftime('%Y-%m','now','localtime')
        GROUP BY t.category ORDER BY 2 DESC)");
    q.bindValue(":uid", m_userId);

    auto* series = new QPieSeries;
    series->setHoleSize(0.35); // 도넛 스타일
    if (q.exec()) {
        while (q.next())
            series->append(q.value(0).toString(), q.value(1).toDouble());
    }
    if (series->count() == 0) series->append("데이터 없음", 1);

    // 레이블 표시
    for (auto* slice : series->slices()) {
        slice->setLabelVisible(slice->percentage() > 0.05);
        slice->setLabelPosition(QPieSlice::LabelOutside);
    }

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setTitle("이번 달 카테고리별 지출");
    chart->setTitleFont(QFont("맑은 고딕", 10, QFont::Bold));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false);
    chart->legend()->setAlignment(Qt::AlignBottom);

    static_cast<QChartView*>(m_pieChartView)->setChart(chart);
}

void DashboardWidget::fetchExchangeRates() {
    m_rateUpdateLabel->setText("환율 불러오는 중...");
    auto* nam   = new QNetworkAccessManager(this);
    // base=KRW → 1 KRW = rates["USD"] USD, rates["JPY"] JPY, ...
    auto* reply = nam->get(QNetworkRequest(QUrl("https://open.er-api.com/v6/latest/KRW")));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_rateUpdateLabel->setText("⚠ 환율 정보를 가져올 수 없습니다");
            m_usdLabel->setText("—"); m_jpyLabel->setText("—");
            m_eurLabel->setText("—"); m_cnyLabel->setText("—");
            return;
        }

        auto doc   = QJsonDocument::fromJson(reply->readAll());
        auto rates = doc.object()["rates"].toObject();

        // 1 KRW = rates["USD"] USD  →  1 USD = 1/rates["USD"] KRW
        auto krwPer = [&](const QString& code) -> double {
            double r = rates[code].toDouble();
            return r > 0 ? 1.0 / r : 0.0;
        };

        auto fmt = [](double v) {
            return AccountModel::formatKRW(qRound(v));
        };

        m_usdLabel->setText(fmt(krwPer("USD")));
        m_jpyLabel->setText(fmt(krwPer("JPY") * 100)); // 100엔 기준
        m_eurLabel->setText(fmt(krwPer("EUR")));
        m_cnyLabel->setText(fmt(krwPer("CNY")));

        m_rateUpdateLabel->setText(
            "● 마지막 업데이트: " +
            QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        m_rateUpdateLabel->setStyleSheet("color:#10B981; font-size:9pt; font-weight:600;");
    });
}
