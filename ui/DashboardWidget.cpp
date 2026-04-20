#include "DashboardWidget.h"
#include "DatabaseManager.h"
#include "AccountModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSqlQuery>
#include <QDateTime>
#include <QSizePolicy>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QPainter>
#include <QPushButton>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>
#include <QtWidgets>

// ─── 요약 카드 ───────────────────────────────────────────────
static QGroupBox* makeSummaryCard(const QString& title, QLabel*& valLabel,
                                  const QString& accent, QWidget* parent) {
    auto* box = new QGroupBox(parent);
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    box->setFixedHeight(94);
    box->setStyleSheet(QString(
        "QGroupBox { border: 1px solid #E5E7EB; border-left: 4px solid %1; "
        "border-radius: 10px; background: #FFFFFF; margin-top: 0; }").arg(accent));

    auto* titleLbl = new QLabel(title, box);
    titleLbl->setStyleSheet("color:#6B7280; font-size:8.5pt; font-weight:600; background:transparent;");

    valLabel = new QLabel("₩0", box);
    valLabel->setStyleSheet(
        QString("color:%1; font-size:17pt; font-weight:700; "
                "letter-spacing:-0.5px; background:transparent;").arg(accent));
    valLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* l = new QVBoxLayout(box);
    l->setContentsMargins(16, 10, 16, 10);
    l->setSpacing(4);
    l->addWidget(titleLbl);
    l->addWidget(valLabel);
    return box;
}

// ─── 환율 카드 ───────────────────────────────────────────────
static QGroupBox* makeRateCard(const QString& code, const QString& name,
                                const QString& flag, QLabel*& valLabel,
                                QWidget* parent) {
    auto* box = new QGroupBox(parent);
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    box->setStyleSheet(
        "QGroupBox { border: 1px solid #E5E7EB; border-radius: 10px; "
        "background: #FFFFFF; margin-top: 0; }");

    auto* flagLbl = new QLabel(flag, box);
    flagLbl->setStyleSheet("font-size:16pt; background:transparent; padding:0;");

    auto* codeLbl = new QLabel(code, box);
    codeLbl->setStyleSheet(
        "font-size:12pt; font-weight:700; color:#111827; background:transparent;");

    auto* nameLbl = new QLabel(name, box);
    nameLbl->setStyleSheet("font-size:8pt; color:#6B7280; background:transparent;");

    valLabel = new QLabel("—", box);
    valLabel->setStyleSheet(
        "font-size:14pt; font-weight:700; color:#2563EB; background:transparent;");

    auto* unitLbl = new QLabel(code == "JPY" ? "100엔 기준" : "1단위 기준", box);
    unitLbl->setStyleSheet("font-size:7.5pt; color:#9CA3AF; background:transparent;");

    auto* topRow = new QHBoxLayout;
    topRow->setSpacing(6);
    topRow->addWidget(flagLbl);
    topRow->addWidget(codeLbl);
    topRow->addStretch();

    auto* l = new QVBoxLayout(box);
    l->setContentsMargins(14, 10, 14, 10);
    l->setSpacing(2);
    l->addLayout(topRow);
    l->addWidget(nameLbl);
    l->addSpacing(6);
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
    m_rateTimer->setInterval(5 * 60 * 1000);
    connect(m_rateTimer, &QTimer::timeout, this, &DashboardWidget::fetchExchangeRates);
    m_rateTimer->start();

    refresh();
}

void DashboardWidget::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(12);

    // ── 1. 요약 카드 ─────────────────────────────────────────
    auto* cardsRow = new QHBoxLayout;
    cardsRow->setSpacing(12);
    cardsRow->addWidget(makeSummaryCard("총 자산",      m_totalBalanceLabel, "#2563EB", this));
    cardsRow->addWidget(makeSummaryCard("이번 달 수입", m_monthIncomeLabel,  "#059669", this));
    cardsRow->addWidget(makeSummaryCard("이번 달 지출", m_monthExpenseLabel, "#DC2626", this));
    root->addLayout(cardsRow);

    // ── 2. 환율 섹션 ─────────────────────────────────────────
    auto* rateGroup  = new QGroupBox("실시간 환율", this);
    rateGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* rateLayout = new QVBoxLayout(rateGroup);
    rateLayout->setContentsMargins(12, 8, 12, 12);
    rateLayout->setSpacing(8);

    auto* rateHeader = new QHBoxLayout;
    m_rateUpdateLabel = new QLabel("업데이트 중...", this);
    m_rateUpdateLabel->setStyleSheet("color:#9CA3AF; font-size:8.5pt;");
    auto* refreshBtn = new QPushButton("↻ 새로고침", this);
    refreshBtn->setFixedSize(84, 26);
    refreshBtn->setStyleSheet(
        "QPushButton { background:#EFF6FF; color:#2563EB; border:none; "
        "border-radius:5px; font-size:8.5pt; font-weight:600; }"
        "QPushButton:hover { background:#DBEAFE; }");
    connect(refreshBtn, &QPushButton::clicked, this, &DashboardWidget::fetchExchangeRates);
    rateHeader->addWidget(m_rateUpdateLabel);
    rateHeader->addStretch();
    rateHeader->addWidget(refreshBtn);
    rateLayout->addLayout(rateHeader);

    auto* rateCards = new QHBoxLayout;
    rateCards->setSpacing(10);
    rateCards->addWidget(makeRateCard("USD", "미국 달러", "🇺🇸", m_usdLabel, this));
    rateCards->addWidget(makeRateCard("JPY", "일본 엔",   "🇯🇵", m_jpyLabel, this));
    rateCards->addWidget(makeRateCard("EUR", "유로",      "🇪🇺", m_eurLabel, this));
    rateCards->addWidget(makeRateCard("CNY", "중국 위안", "🇨🇳", m_cnyLabel, this));
    rateLayout->addLayout(rateCards);
    root->addWidget(rateGroup);

    // ── 3. 차트 ──────────────────────────────────────────────
    setupCharts();

    auto* barGroup = new QGroupBox("월별 수입 / 지출", this);
    barGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* barLay = new QVBoxLayout(barGroup);
    barLay->setContentsMargins(8, 4, 8, 8);
    barLay->addWidget(m_barChartView);

    auto* pieGroup = new QGroupBox("이번 달 카테고리별 지출", this);
    pieGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* pieLay = new QVBoxLayout(pieGroup);
    pieLay->setContentsMargins(8, 4, 8, 8);
    pieLay->addWidget(m_pieChartView);

    auto* chartRow = new QHBoxLayout;
    chartRow->setSpacing(12);
    chartRow->addWidget(barGroup, 3);
    chartRow->addWidget(pieGroup, 2);
    root->addLayout(chartRow, 1); // stretch=1 → 남은 공간 모두 차트에
}

void DashboardWidget::setupCharts() {
    auto* barView = new QChartView(this);
    auto* pieView = new QChartView(this);
    barView->setRenderHint(QPainter::Antialiasing);
    pieView->setRenderHint(QPainter::Antialiasing);
    barView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pieView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    barView->setMinimumHeight(180);
    pieView->setMinimumHeight(180);
    barView->setFrameShape(QFrame::NoFrame);
    pieView->setFrameShape(QFrame::NoFrame);
    m_barChartView = barView;
    m_pieChartView = pieView;
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
    incomeSet->setColor(QColor("#059669"));
    incomeSet->setBorderColor(Qt::transparent);
    expenseSet->setColor(QColor("#EF4444"));
    expenseSet->setBorderColor(Qt::transparent);
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
    series->setBarWidth(0.6);

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->setMargins(QMargins(4, 4, 4, 4));

    auto* legend = chart->legend();
    legend->setAlignment(Qt::AlignTop);
    legend->setFont(QFont("맑은 고딕", 8));
    legend->setLabelColor(QColor("#374151"));
    legend->setMarkerShape(QLegend::MarkerShapeRectangle);

    auto* axisX = new QBarCategoryAxis;
    axisX->append(months);
    axisX->setLabelsFont(QFont("맑은 고딕", 8));
    axisX->setLabelsColor(QColor("#6B7280"));
    axisX->setGridLineVisible(false);
    axisX->setLinePenColor(QColor("#E5E7EB"));
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto* axisY = new QValueAxis;
    axisY->setLabelsFont(QFont("맑은 고딕", 8));
    axisY->setLabelsColor(QColor("#6B7280"));
    axisY->setGridLineColor(QColor("#F3F4F6"));
    axisY->setLinePenColor(Qt::transparent);
    axisY->setTickCount(5);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    static_cast<QChartView*>(m_barChartView)->setChart(chart);
}

void DashboardWidget::updatePieChart() {
    static const QString kColors[] = {
        "#3B82F6", "#059669", "#F59E0B", "#EF4444",
        "#8B5CF6", "#06B6D4", "#F97316", "#EC4899"
    };

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
    series->setHoleSize(0.38);
    series->setPieSize(0.78);
    if (q.exec()) {
        while (q.next())
            series->append(q.value(0).toString(), q.value(1).toDouble());
    }
    if (series->count() == 0) series->append("데이터 없음", 1);

    int i = 0;
    for (auto* slice : series->slices()) {
        slice->setColor(QColor(kColors[i % 8]));
        slice->setBorderColor(QColor("#FFFFFF"));
        slice->setBorderWidth(2);
        slice->setLabelVisible(slice->percentage() > 0.05);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setLabelFont(QFont("맑은 고딕", 8));
        slice->setLabelColor(QColor("#374151"));
        ++i;
    }

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false);
    chart->setPlotAreaBackgroundVisible(false);
    chart->setMargins(QMargins(4, 4, 4, 4));

    auto* legend = chart->legend();
    legend->setAlignment(Qt::AlignBottom);
    legend->setFont(QFont("맑은 고딕", 8));
    legend->setLabelColor(QColor("#374151"));
    legend->setMarkerShape(QLegend::MarkerShapeCircle);

    static_cast<QChartView*>(m_pieChartView)->setChart(chart);
}

void DashboardWidget::fetchExchangeRates() {
    m_rateUpdateLabel->setText("환율 불러오는 중...");
    m_rateUpdateLabel->setStyleSheet("color:#9CA3AF; font-size:8.5pt;");
    auto* nam   = new QNetworkAccessManager(this);
    auto* reply = nam->get(QNetworkRequest(QUrl("https://open.er-api.com/v6/latest/KRW")));

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_rateUpdateLabel->setText("⚠ 환율 정보를 가져올 수 없습니다");
            m_rateUpdateLabel->setStyleSheet("color:#EF4444; font-size:8.5pt;");
            m_usdLabel->setText("—"); m_jpyLabel->setText("—");
            m_eurLabel->setText("—"); m_cnyLabel->setText("—");
            return;
        }

        auto doc   = QJsonDocument::fromJson(reply->readAll());
        auto rates = doc.object()["rates"].toObject();

        auto krwPer = [&](const QString& code) -> double {
            double r = rates[code].toDouble();
            return r > 0 ? 1.0 / r : 0.0;
        };
        auto fmt = [](double v) { return AccountModel::formatKRW(qRound(v)); };

        m_usdLabel->setText(fmt(krwPer("USD")));
        m_jpyLabel->setText(fmt(krwPer("JPY") * 100));
        m_eurLabel->setText(fmt(krwPer("EUR")));
        m_cnyLabel->setText(fmt(krwPer("CNY")));

        m_rateUpdateLabel->setText(
            "● 마지막 업데이트: " +
            QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        m_rateUpdateLabel->setStyleSheet(
            "color:#059669; font-size:8.5pt; font-weight:600;");
    });
}
